/*
 * This is the software that will run on the Arduino Due in the chessboard project.
 * 
 * Initially we're aiming to have the board simply play chess against itself. The Pi will send instructions for the moves and the Arduino will
 * respond with a DONE once that instruction has been carried out. 
 * 
 * 
 * To do next:
 *  - En Passant
 *  - Promotion
 *  - Zero Head!
 * 
 * Functions:
 *  - Communication with Pi. Wait for a command from the Pi (me2e4) and transmit "DONE" when completed.
 * 
 *  - Raise and Lower Magnet Head
 *  
 *  - Zero the Magnet Head
 *  
 *  - Move Magnet Head to Position as quickly and smoothly as possible(X, Y)
 *  
 *  - Move Piece from square (A,B) to (C,D). 
 *      # If there's a piece at (C,D) remove it to the side first - Call a function Remove_Piece()
 *      # If the (A,B) piece is not a knight, move the piece directly to (C,D)
 *      # If the (A,B) piece is a knight do a 3 part move between and might be pieces
 *      
 *  - Remove_Piece(X,Y) - Move piece off the board to the next available side space
 *  
 *  - Setup_Board(FEN) - Move Pieces from the current board position, to the one issued in FEN 
 *      # Goes through each board square in FEN, checks if the required piece is in position, if it isn't, it runs Remove_Piece() if there's an occupying piece,
 *        locates the needed piece, and moves it to position.
 *  
 *  - Castle
 *  
 *  - Promote
 * 
 */
 
//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
#include "Pins.h"
#include "Support.h"
#include "AccelStepper.h"
#include <Servo.h>

char  Piece_Track_Buffer[10][10];                                                 // This buffer stores what peiece is in each square 
byte  Graphics_Buffer[8][8][3];                                                   // This buffer stores the RGB values for each square
byte  MagnetoSensor_Buffer[10][10];                                               // This buffer stores chequers with a piece on them with a 1
byte  MagnetoSensor_BufferStore[10][10];                                          // This is a copy of the previous buffer used to detect changes

char  Command_buffer[5];

byte  Timer_Ticks = 0;
byte  Update_MangetoSensor_Positions; 

AccelStepper Xstepper(AccelStepper::DRIVER, X_Stepper_Step, X_Stepper_Dir);       // Define some steppers and the pins that will use
AccelStepper Ystepper(AccelStepper::DRIVER, Y_Stepper_Step, Y_Stepper_Dir);
Servo servo1;

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void setup() 
{ 
  Configure_Pins();
  //Zero_Head();
  
  Serial.begin(115200);
  Serial.setTimeout(1000);

  servo1.attach(servo_pin); 
  
  Piece_Track_Buffer_Initalise(); 
  //Print_Piece_Track_Buffer();

  Xstepper.setMaxSpeed(30000.0);
  Xstepper.setAcceleration(30000.0);  
  Xstepper.setPinsInverted(1,0,0);                                                // Invert X axis  
  
  Ystepper.setMaxSpeed(30000.0);
  Ystepper.setAcceleration(30000.0); 

  Disengage_Head();
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void loop() 
{   
  byte Valid_Command = false;                                                     // We haven't received a command yet so of course its false!
  int Coord_X, Coord_Y, Coord_U, Coord_V;                                         // Store the coordinates of positions ie. (4,6) to (4,5) etc
  long X, Y, U, V;                                                                // Store the step coordinates used by motors ie. (7100, 14200) to etc
   
  while(1)  
  {   
    // 1) --------------- Obtain a valid command over the serial port ------------ 
    //Serial.println("Please enter a move (eg. me2e4): ");
    Valid_Command = false;
    while(!Valid_Command)                                                         // Loop until we receive a valid command
    {
      while(!Get_Command())                                                       // Wait for a command
      {}
    
      //Serial.print("Command Received: ");                                         // Display whatever we have just received
      //Serial.println(Command_buffer);
       
      Coord_X = (Command_buffer[0] - 96);                                         // Convert Command into coordinates to move from and to:                          
      Coord_Y = (Command_buffer[1] - 48);                                         // 1) Convert ASCII characters into a 0-9 value
      Coord_U = (Command_buffer[2] - 96);                                         
      Coord_V = (Command_buffer[3] - 48);
      
      X = Coord_X * STEPS_PER_SQUARE;                                             // 2) Transform coordinates into stepper motor steps
      Y = Coord_Y * STEPS_PER_SQUARE;
      U = Coord_U * STEPS_PER_SQUARE;
      V = Coord_V * STEPS_PER_SQUARE;    
                                  
      if(X >= STEPS_MIN and X <= STEPS_MAX) {                                     // Check command is valid  
        if(Y >= STEPS_MIN and Y <= STEPS_MAX) {
          if(U >= STEPS_MIN and U <= STEPS_MAX) {
            if(V >= STEPS_MIN and V <= STEPS_MAX) {
              //Serial.println("Valid coordinates!");              
              Valid_Command = true;
            }  
          }  
        }  
      }
      
      if(!Valid_Command)  {
        //Serial.println("Invalid coordinates!");
        //Serial.println("Please enter a move (eg. me2e4): ");
        Move_Head(0, 0);  
      }
    }                                                                             // If a valid command, continue. Otherwise loop back and wait for a new command

    // 2) --------------- Move the stepper motors to move the pieces -------------
    //Serial.print("Coordinates to move from: (");
    //Serial.print(Coord_X); Serial.print(", "); Serial.print(Coord_Y); Serial.println(")");  
    //Serial.print("Coordinates to move to: (");
    //Serial.print(Coord_U); Serial.print(", "); Serial.print(Coord_V); Serial.println(")"); 

    // A) DO WE HAVE TO REMOVE A PIECE FIRST?
    if(Piece_Track_Buffer[Coord_U][Coord_V] != '0') {                            
      //Serial.println("Removing piece first...");                        
      Remove_Piece(Coord_U, Coord_V);                                           
    }
    
    // B) IS THIS MOVE A WHITE CASTLE?
    if(Piece_Track_Buffer[Coord_X][Coord_Y] == 'K' and abs(Coord_U - Coord_X) > 1)                                            
    {                                                
      //Serial.println("White is castling!");
      Move_Head(X, Y);          
      Engage_Head();
      Move_Head_Extra(U, V);
      Disengage_Head(); 
      Update_Position_Table(Coord_X, Coord_Y, Coord_U, Coord_V);

      if(Coord_U > Coord_X) {                                                     // Right side (king's side)
        Move_Head(STEPS_PER_SQUARE * 8, STEPS_PER_SQUARE);
        Engage_Head();
        Move_Head(STEPS_PER_SQUARE * 8, STEPS_PER_SQUARE/2);
        Move_Head(STEPS_PER_SQUARE * 6, STEPS_PER_SQUARE/2); 
        Move_Head_Extra(STEPS_PER_SQUARE * 6, STEPS_PER_SQUARE);
        Disengage_Head();
        Update_Position_Table(8, 1, 6, 1);          
      }
      else  {                                                                     // left side (queen's side)
        Move_Head(STEPS_PER_SQUARE, STEPS_PER_SQUARE);
        Engage_Head();
        Move_Head(STEPS_PER_SQUARE, STEPS_PER_SQUARE/2);
        Move_Head(STEPS_PER_SQUARE * 4, STEPS_PER_SQUARE/2); 
        Move_Head_Extra(STEPS_PER_SQUARE * 4, STEPS_PER_SQUARE);
        Disengage_Head();
        Update_Position_Table(1, 1, 4, 1);          
      }
    }

    // C) IS THIS MOVE A BLACK CASTLE?
    else if(Piece_Track_Buffer[Coord_X][Coord_Y] == 'k' and abs(Coord_U - Coord_X) > 1)                                           
    {                                                
      //Serial.println("Black is castling!");
      Move_Head(X, Y);          
      Engage_Head();
      Move_Head_Extra(U, V);
      Disengage_Head(); 
      Update_Position_Table(Coord_X, Coord_Y, Coord_U, Coord_V);

      if(Coord_U > Coord_X) {                                                     // Right side (king's side)
        Move_Head(STEPS_PER_SQUARE * 8, STEPS_PER_SQUARE * 8);
        Engage_Head();
        Move_Head(STEPS_PER_SQUARE * 8, ((17 * STEPS_PER_SQUARE)/2));
        Move_Head(STEPS_PER_SQUARE * 6, ((17 * STEPS_PER_SQUARE)/2)); 
        Move_Head_Extra(STEPS_PER_SQUARE * 6, STEPS_PER_SQUARE * 8);
        Disengage_Head();
        Update_Position_Table(8, 8, 6, 8);          
      }
      else  {                                                                     // left side (queen's side)
        Move_Head(STEPS_PER_SQUARE, STEPS_PER_SQUARE * 8);
        Engage_Head();
        Move_Head(STEPS_PER_SQUARE, ((17 * STEPS_PER_SQUARE)/2));
        Move_Head(STEPS_PER_SQUARE * 4, ((17 * STEPS_PER_SQUARE)/2)); 
        Move_Head_Extra(STEPS_PER_SQUARE * 4, STEPS_PER_SQUARE * 8);
        Disengage_Head();
        Update_Position_Table(1, 8, 4, 8);          
      }              
    }
    
    // D) ARE WE MOVING A KNIGHT?
    else if(Piece_Track_Buffer[Coord_X][Coord_Y] == 'n' or                        
       Piece_Track_Buffer[Coord_X][Coord_Y] == 'N') 
    {                            
      //Serial.println("We're moving a knight!");
      Move_Head(X, Y);
      Engage_Head();
      
      if(Coord_U - Coord_X > 0) {                                                 // Moving the castle rightwards
        if(Coord_V - Coord_Y == 2)  {                                               
          Move_Head(X + (STEPS_PER_SQUARE/2), Y + (STEPS_PER_SQUARE/2));
          Move_Head(X + (STEPS_PER_SQUARE/2), V - (STEPS_PER_SQUARE/2));    
        }
        else if(Coord_V - Coord_Y == 1)  {
          Move_Head(X + (STEPS_PER_SQUARE/2), Y + (STEPS_PER_SQUARE/2));  
          Move_Head(U - (STEPS_PER_SQUARE/2), Y + (STEPS_PER_SQUARE/2));
        }
        else if(Coord_V - Coord_Y == -1)  {
          Move_Head(X + (STEPS_PER_SQUARE/2), Y - (STEPS_PER_SQUARE/2));
          Move_Head(U - (STEPS_PER_SQUARE/2), Y - (STEPS_PER_SQUARE/2));  
        }
        else {
          Move_Head(X + (STEPS_PER_SQUARE/2), Y - (STEPS_PER_SQUARE/2));
          Move_Head(X + (STEPS_PER_SQUARE/2), V + (STEPS_PER_SQUARE/2));  
        }
      }
      else  {                                                                     // Moving the castle leftwards
         if(Coord_V - Coord_Y == 2)  {                                               
          Move_Head(X - (STEPS_PER_SQUARE/2), Y + (STEPS_PER_SQUARE/2));
          Move_Head(X - (STEPS_PER_SQUARE/2), V - (STEPS_PER_SQUARE/2));    
        }
        else if(Coord_V - Coord_Y == 1)  {
          Move_Head(X - (STEPS_PER_SQUARE/2), Y + (STEPS_PER_SQUARE/2));  
          Move_Head(U + (STEPS_PER_SQUARE/2), Y + (STEPS_PER_SQUARE/2));
        }
        else if(Coord_V - Coord_Y == -1)  {
          Move_Head(X - (STEPS_PER_SQUARE/2), Y - (STEPS_PER_SQUARE/2));
          Move_Head(U + (STEPS_PER_SQUARE/2), Y - (STEPS_PER_SQUARE/2));  
        }
        else {
          Move_Head(X - (STEPS_PER_SQUARE/2), Y - (STEPS_PER_SQUARE/2));
          Move_Head(X - (STEPS_PER_SQUARE/2), V + (STEPS_PER_SQUARE/2));  
        }        
      }     
                              
      Move_Head_Extra(U, V);   
      Disengage_Head(); 
      Update_Position_Table(Coord_X, Coord_Y, Coord_U, Coord_V);                          
    }

    // E) JUST AN ORDINARY MOVE THEN!
    else  { 
      Move_Head(X, Y);          
      Engage_Head();                              
      Move_Head_Extra(U, V);   
      Disengage_Head();
      Update_Position_Table(Coord_X, Coord_Y, Coord_U, Coord_V);           
    }

    Serial.print("D");    
    //Print_Piece_Track_Buffer();
  }                                                                               // Command carried out, go wait for the next command          
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
byte Get_Command()
{
  for(int i = 0;i < 5;i++)  {                                                     // Clear buffer
    Command_buffer[i] = 0;    
  }
  
  if(Serial.available() > 0) {                                                    // If there's serial data available...
    Serial.find("m");                                                             // Search for the beginning of the move code. ie. scrap any rubbish...
    Serial.readBytes(Command_buffer, 4);                                          // Read the next 4 bytes which should contain the move data ie. e2e4
  }

  if(Command_buffer[0])  {                                                        // Verify that we've received something
    return(1);
  }
  else  {
    return(0);
  }
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Piece_Track_Buffer_Initalise()
{  
  for(int x = 0; x <= 9; x++)  {
    for(int y = 0; y <= 9; y++)  {
      Piece_Track_Buffer[x][y] = '0';
    }    
  }
  
  Piece_Track_Buffer[1][1] = 'R';
  Piece_Track_Buffer[2][1] = 'N';
  Piece_Track_Buffer[3][1] = 'B'; 
  Piece_Track_Buffer[4][1] = 'Q';
  Piece_Track_Buffer[5][1] = 'K';
  Piece_Track_Buffer[6][1] = 'B';
  Piece_Track_Buffer[7][1] = 'N';
  Piece_Track_Buffer[8][1] = 'R';

  Piece_Track_Buffer[1][8] = 'r';
  Piece_Track_Buffer[2][8] = 'n';
  Piece_Track_Buffer[3][8] = 'b';
  Piece_Track_Buffer[4][8] = 'q';
  Piece_Track_Buffer[5][8] = 'k';
  Piece_Track_Buffer[6][8] = 'b';
  Piece_Track_Buffer[7][8] = 'n';
  Piece_Track_Buffer[8][8] = 'r';

  for(int x = 1; x <= 8; x++)  {
    Piece_Track_Buffer[x][2] = 'P';
    Piece_Track_Buffer[x][7] = 'p';    
  }
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Print_Piece_Track_Buffer()
{
  //Serial.println("Piece Track Buffer: ");
  
  for(int y = 9; y >= 0; y--)  {
    for(int x = 0; x <= 9; x++)  {
      Serial.write(Piece_Track_Buffer[x][y]);
      Serial.print(" ");
    }    
    Serial.println(" ");
  }
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Move_Head(long A, long B)
{   
  A += X_OFFSET;
  B += Y_OFFSET;
  
  A = constrain(A, STEPS_MIN, STEPS_MAX);
  B = constrain(B, STEPS_MIN, STEPS_MAX);  
        
  Xstepper.moveTo(A);
  Ystepper.moveTo(B);
  
  while(Xstepper.distanceToGo() or Ystepper.distanceToGo()) {
    Xstepper.run();  
    Ystepper.run();
  }
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Move_Head_Extra(long A, long B)
{   
  long Was_X = Xstepper.currentPosition();
  long Was_Y = Ystepper.currentPosition();     
  
  A += X_OFFSET;
  B += Y_OFFSET;

  long X_Diff = A - Was_X;
  long Y_Diff = B - Was_Y;

  long Mag_Diff = sqrt((X_Diff*X_Diff) + (Y_Diff*Y_Diff));
  A += (X_Diff * STEPS_EXTRA) / Mag_Diff;
  B += (Y_Diff * STEPS_EXTRA) / Mag_Diff;
  
  A = constrain(A, STEPS_MIN, STEPS_MAX);
  B = constrain(B, STEPS_MIN, STEPS_MAX);  
        
  Xstepper.moveTo(A);
  Ystepper.moveTo(B);
  
  while(Xstepper.distanceToGo() or Ystepper.distanceToGo()) {
    Xstepper.run();  
    Ystepper.run();
  }
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Engage_Head()
{  
  servo1.write(70);  
  delay(500);
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Disengage_Head()
{  
  servo1.write(175); 
  delay(500);  
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Update_Position_Table(int Coord_X, int Coord_Y, int Coord_U, int Coord_V)
{
  Piece_Track_Buffer[Coord_U][Coord_V] = Piece_Track_Buffer[Coord_X][Coord_Y];
  Piece_Track_Buffer[Coord_X][Coord_Y] = '0';                                     // What was at X,Y has now been moved to U,V. X,Y has been left empty
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Remove_Piece(byte Coord_U, byte Coord_V)
{
  // Coord_U, Coord_V hold the position of the piece to remove. Coord_X, Coord_Y hold position to take piexe to
  // 1) What colour is the piece to remove, 2) find next available square on sidelines, 3) perform movements to get piece there, 4) Update position table
  
  int Taken_Piece, Coord_X, Coord_Y;    
  
  // 1)  --------------- What colour is the piece to remove? ---------------------   
  if(Piece_Track_Buffer[Coord_U][Coord_V] < 'Z')  {
    Taken_Piece = WHITE;
  }
  else  {
    Taken_Piece = BLACK;
  }
  
  // 2)  --------------- Find the next available square on the sidelines --------- 
  if(Taken_Piece == WHITE)  
  {
    Coord_X = 0;
    Coord_Y = 9;
    
    while(Coord_Y >= 0)                                                           // Search from top left down to bottom left for a free square
    {
      if(Piece_Track_Buffer[0][Coord_Y] == '0') {
        break;
      }
      Coord_Y--;    
    }

    if(Coord_Y < 0) {                                                             // If there weren't any free squares on the left, search from bottom left to bottom right
      Coord_Y = 0;
      while(Coord_X <= 9)                                                                 
      {
        if(Piece_Track_Buffer[Coord_X][0] == '0') {
          break;
        }
        Coord_X++;    
      }
    }

    if(Coord_X == 10) {                                                           // By the time we reach here, we should have the x,y coordinates of the next free slot on white's sidelines
      //Serial.println("There's been a problem searching for a free spot on white's sidelines!");
      return;
    }
  }
  
  else  
  {
    Coord_X = 9;
    Coord_Y = 0;
    
    while(Coord_Y <= 9)                                                           // Search from bottom right up to top right for a free square
    {
      if(Piece_Track_Buffer[9][Coord_Y] == '0') {
        break;
      }
      Coord_Y++;    
    }

    if(Coord_Y > 9) {                                                             // If there weren't any free squares on the right, search from top right to top left
      Coord_Y = 9;
      while(Coord_X >= 0)                                                                 
      {
        if(Piece_Track_Buffer[Coord_X][9] == '0') {
          break;
        }
        Coord_X--;    
      }
    }

    if(Coord_X < 0) {                                                             // By the time we reach here, we should have the x,y coordinates of the next free slot on black's sidelines
      //Serial.println("There's been a problem searching for a free spot on black's sidelines!");
      return;
    }    
  }

  //Serial.print("Position to place removed piece is: (");
  //Serial.print(Coord_X); Serial.print(", "); Serial.print(Coord_Y); Serial.println(")");

  // 3)  --------------- Perform the movements to get the piece there ------------
  long X = Coord_U * STEPS_PER_SQUARE;                                            // Initial square
  long Y = Coord_V * STEPS_PER_SQUARE;
    
  long U = Coord_X * STEPS_PER_SQUARE;                                            // Final square
  long V = Coord_Y * STEPS_PER_SQUARE;

  if(Coord_X == 0)                                                                // Left sideline
  {  
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X, Y + (STEPS_PER_SQUARE/2));                                       // Move to (X, Y+0.5)          
    Move_Head((STEPS_PER_SQUARE/2), Y + (STEPS_PER_SQUARE/2));                    // Move to (0.5, Y+0.5):         
    Move_Head((STEPS_PER_SQUARE/2), V);                                           // Move to (0.5, V)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();      
  }
  else if(Coord_X == 9)                                                           // Right sideline
  {
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X, Y + (STEPS_PER_SQUARE/2));                                       // Move to (X, Y+0.5)          
    Move_Head(((17 * STEPS_PER_SQUARE)/2), Y + (STEPS_PER_SQUARE/2));             // Move to (8.5, Y+0.5):         
    Move_Head(((17 * STEPS_PER_SQUARE)/2), V);                                    // Move to (8.5, V)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();    
  }
  else if(Coord_Y == 0)                                                           // Bottom sideline
  {
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X + (STEPS_PER_SQUARE/2), Y);                                       // Move to (X+0.5, Y)          
    Move_Head(X + (STEPS_PER_SQUARE/2), (STEPS_PER_SQUARE/2));                    // Move to (X+0.5, 0.5):         
    Move_Head(U, (STEPS_PER_SQUARE/2));                                           // Move to (U, 0.5)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();    
  }
  else if(Coord_Y == 9)                                                           // Top sideline
  {
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X + (STEPS_PER_SQUARE/2), Y);                                       // Move to (X+0.5, Y)          
    Move_Head(X + (STEPS_PER_SQUARE/2), ((17 * STEPS_PER_SQUARE)/2));             // Move to (X+0.5, 8.5):         
    Move_Head(U, ((17 * STEPS_PER_SQUARE)/2));                                    // Move to (U, 8.5)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();    
  }

  // 4)) Update position table
  Update_Position_Table(Coord_U, Coord_V, Coord_X, Coord_Y);
}














































//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Copy_MagnetoSensor_Buffer()
{
  for(int x = 0; x <= 10; x++)  {
    for(int y = 0; y <= 10; y++)  {
      MagnetoSensor_BufferStore[x][y] = MagnetoSensor_Buffer[x][y];
    }    
  }
}

//--------------------------------------------------------------------------------
//### LED DISPLAY ROUTINE ########################################################
//--------------------------------------------------------------------------------
void Timer_Interrupt()
{
  static int Activated_Ground_Line = 0;  
  Timer_Ticks += 16;                                                              // Timer_Ticks is a byte and overflows back to 0 after 16 additions

  if(!Timer_Ticks)                                                                // 16 ticks have passed, time to shift the ground line on by one. 
  { 
    digitalWrite((Darlington_Pin_0 + Activated_Ground_Line), LOW);                // Deactivate current ground

    // Turn on all lines that have a non-zero colour intensity and switch the rest off:
    for(int y = 0; y < 8; y++)  {
      if(Graphics_Buffer[Activated_Ground_Line][y][RED] >= 16) {
        digitalWrite((R_Channel_0 + y), HIGH);
      }
      else  {
        digitalWrite((R_Channel_0 + y), LOW);
      }
      if(Graphics_Buffer[Activated_Ground_Line][y][GREEN] >= 16) {
        digitalWrite((G_Channel_0 + y), HIGH);
      }
      else  {
        digitalWrite((G_Channel_0 + y), LOW);
      }
      if(Graphics_Buffer[Activated_Ground_Line][y][BLUE] >= 16) {
        digitalWrite((B_Channel_0 + y), HIGH);
      }
      else  {
        digitalWrite((B_Channel_0 + y), LOW);
      }
    } 

    Activated_Ground_Line++;                                                      // Increment the count
    if(Activated_Ground_Line >= 8)  {                                             // If we've reached the end, loop back to zero
      Activated_Ground_Line = 0;     
    } 
      
    digitalWrite((Darlington_Pin_0 + Activated_Ground_Line), HIGH);               // Make our new ground line grounded.   
    
    return;                                                                        
  }

  // Make lines go low when Timer_Ticks exceeds the intensity value for each channel:
  for(int y = 0; y < 8; y++)  {                                             
    if(Graphics_Buffer[Activated_Ground_Line][y][RED] < Timer_Ticks)  {
      digitalWrite(R_Channel_0, LOW);
    }
    if(Graphics_Buffer[Activated_Ground_Line][y][GREEN] < Timer_Ticks)  {
      digitalWrite(G_Channel_0, LOW);
    }
    if(Graphics_Buffer[Activated_Ground_Line][y][BLUE] < Timer_Ticks)  {
      digitalWrite(B_Channel_0, LOW);
    }
  }

  // We read the Magnetoresistive sensors 1/2 through the activation of a particular ground line (if required)
  if(Update_MangetoSensor_Positions and Timer_Ticks == 128)  {
    for(int y = 0; y < 8; y++)  {
      MagnetoSensor_Buffer[Activated_Ground_Line][y] = digitalRead(Mag_Sense_0 + y);
    }    
  }
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
















