/*
 * This is the software that will run on the Arduino Due in the chessboard project.
 * 
 * To do next:
 *  - En Passant
 *  - Promotion
 * 
 * Functions:
 *  - Communication with Pi. Wait for a command from the Pi (me2e4) and transmit "DONE" when completed.
 *  - Raise and Lower Magnet Head
 *  - Zero the Magnet Head
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
 * 
 */

//#define  _DEBUG
 
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
 
unsigned long Total_Button_Press_Time, Intial_Button_Press_Time;
byte Pause_Flag = 0;
byte Head_Enabled_Flag = 0;

AccelStepper Xstepper(AccelStepper::DRIVER, X_Stepper_Step, X_Stepper_Dir);       // Define some steppers and the pins that will use
AccelStepper Ystepper(AccelStepper::DRIVER, Y_Stepper_Step, Y_Stepper_Dir);
Servo servo1;

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void setup() 
{ 
  Configure_Pins();  
  Xstepper.setPinsInverted(1,0,0);                                                // Invert X axis 
  attachInterrupt(digitalPinToInterrupt(Mode_Button), Button_Press, RISING);  
  Zero_Head();
  
  Serial.begin(115200);
  Serial.setTimeout(1000);

  servo1.attach(servo_pin); 
  
  Piece_Track_Buffer_Initalise(); 
  #ifdef _DEBUG  
    Print_Piece_Track_Buffer();
  #endif 

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
  byte Remove_Piece_Flag = 0;
  long X_CORNER_B, Y_CORNER_B;
  byte Update_Flag = 0;
   
  while(1)  
  {   
    // 1) --------------- Obtain a valid command over the serial port ------------ 
    #ifdef _DEBUG
      Serial.println("Please enter a move (eg. me2e4): ");
    #endif
    
    Valid_Command = false;
    while(!Valid_Command)                                                         // Loop until we receive a valid command
    {
      while(!Get_Command()) {}                                                    // Wait for a command

      #ifdef _DEBUG
        Serial.print("Command Received: ");                                       // Display whatever we have just received
        Serial.println(Command_buffer);
      #endif
       
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
              #ifdef _DEBUG
                Serial.println("Valid coordinates!");
              #endif              
              Valid_Command = true;
            }  
          }  
        }  
      }
      
      if(!Valid_Command)  {
        #ifdef _DEBUG
          Serial.println("Invalid coordinates!");
          Serial.println("Please enter a move (eg. me2e4): ");
        #endif
        Move_Head(0, 0);  
      }
    }                                                                             // If a valid command, continue. Otherwise loop back and wait for a new command

    // 2) --------------- Move the stepper motors to move the pieces -------------
    #ifdef _DEBUG
      Serial.print("Coordinates to move from: (");
      Serial.print(Coord_X); Serial.print(", "); Serial.print(Coord_Y); Serial.println(")");  
      Serial.print("Coordinates to move to: (");
      Serial.print(Coord_U); Serial.print(", "); Serial.print(Coord_V); Serial.println(")"); 
    #endif

    // A) DO WE HAVE TO REMOVE A PIECE FIRST?
    if(Piece_Track_Buffer[Coord_U][Coord_V] != '0') {
      #ifdef _DEBUG                                    
        Serial.println("Removing piece first...");
      #endif 

      Remove_Piece_Flag = 1;      
      if(Coord_U > Coord_X and Coord_V > Coord_Y) {                             // move up and right
        X_CORNER_B = -HALF_SQUARE;
        Y_CORNER_B = -HALF_SQUARE;                          
      }
      else if(Coord_U > Coord_X and Coord_V < Coord_Y) {                        // move down and right
        X_CORNER_B = -HALF_SQUARE;
        Y_CORNER_B = HALF_SQUARE;               
      }
      else if(Coord_U < Coord_X and Coord_V > Coord_Y) {                        // move up and left
        X_CORNER_B = HALF_SQUARE;
        Y_CORNER_B = -HALF_SQUARE;                            
      }
      else if(Coord_U < Coord_X and Coord_V < Coord_Y) {                        // move down and left
        X_CORNER_B = HALF_SQUARE;
        Y_CORNER_B = HALF_SQUARE;                            
      }
      else if(Coord_U < Coord_X and Coord_V == Coord_Y) {                       // move left
        X_CORNER_B = HALF_SQUARE;
        Y_CORNER_B = 0;                            
      }
      else if(Coord_U > Coord_X and Coord_V == Coord_Y) {                       // move right
        X_CORNER_B = -HALF_SQUARE;
        Y_CORNER_B = 0;                            
      } 
      else if(Coord_U == Coord_X and Coord_V < Coord_Y) {                        // move down
        X_CORNER_B = 0;
        Y_CORNER_B = HALF_SQUARE;                            
      }
      else if(Coord_U == Coord_X and Coord_V > Coord_Y) {                        // move up
        X_CORNER_B = 0;
        Y_CORNER_B = -HALF_SQUARE;                            
      }                                                     
    }
    else {
        X_CORNER_B = 0;
        Y_CORNER_B = 0;      
    }
    
    // B) IS THIS MOVE A WHITE CASTLE?
    if(Piece_Track_Buffer[Coord_X][Coord_Y] == 'K' and abs(Coord_U - Coord_X) > 1)                                            
    {     
      #ifdef _DEBUG                                           
        Serial.println("White is castling!");
      #endif
      Move_Head(X, Y);          
      Engage_Head();
      Move_Head_Extra(U, V);
      Disengage_Head(); 
      Update_Position_Table(Coord_X, Coord_Y, Coord_U, Coord_V);

      if(Coord_U > Coord_X) {                                                     // Right side (king's side)
        Move_Head(STEPS_PER_SQUARE * 8, STEPS_PER_SQUARE);
        Engage_Head();
        Move_Head(STEPS_PER_SQUARE * 8, HALF_SQUARE);
        Move_Head(STEPS_PER_SQUARE * 6, HALF_SQUARE); 
        Move_Head_Extra(STEPS_PER_SQUARE * 6, STEPS_PER_SQUARE);
        Disengage_Head();
        Update_Position_Table(8, 1, 6, 1);          
      }
      else  {                                                                     // left side (queen's side)
        Move_Head(STEPS_PER_SQUARE, STEPS_PER_SQUARE);
        Engage_Head();
        Move_Head(STEPS_PER_SQUARE, HALF_SQUARE);
        Move_Head(STEPS_PER_SQUARE * 4, HALF_SQUARE); 
        Move_Head_Extra(STEPS_PER_SQUARE * 4, STEPS_PER_SQUARE);
        Disengage_Head();
        Update_Position_Table(1, 1, 4, 1);          
      }
    }

    // C) IS THIS MOVE A BLACK CASTLE?
    else if(Piece_Track_Buffer[Coord_X][Coord_Y] == 'k' and abs(Coord_U - Coord_X) > 1)                                           
    { 
      #ifdef _DEBUG                                               
        Serial.println("Black is castling!");
      #endif
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
      #ifdef _DEBUG                         
        Serial.println("We're moving a knight!");
      #endif
      Move_Head(X, Y);
      Engage_Head();
      
      if(Coord_U - Coord_X > 0) {                                                 // Moving the castle rightwards
        if(Coord_V - Coord_Y == 2)  {                                               
          Move_Head(X + HALF_SQUARE, Y + HALF_SQUARE);
          Move_Head(X + HALF_SQUARE, V - HALF_SQUARE);    
        }
        else if(Coord_V - Coord_Y == 1)  {
          Move_Head(X + HALF_SQUARE, Y + HALF_SQUARE);  
          Move_Head(U - HALF_SQUARE, Y + HALF_SQUARE);
        }
        else if(Coord_V - Coord_Y == -1)  {
          Move_Head(X + HALF_SQUARE, Y - HALF_SQUARE);
          Move_Head(U - HALF_SQUARE, Y - HALF_SQUARE);  
        }
        else {
          Move_Head(X + HALF_SQUARE, Y - HALF_SQUARE);
          Move_Head(X + HALF_SQUARE, V + HALF_SQUARE);  
        }
      }
      else  {                                                                     // Moving the castle leftwards
         if(Coord_V - Coord_Y == 2)  {                                               
          Move_Head(X - HALF_SQUARE, Y + HALF_SQUARE);
          Move_Head(X - HALF_SQUARE, V - HALF_SQUARE);    
        }
        else if(Coord_V - Coord_Y == 1)  {
          Move_Head(X - HALF_SQUARE, Y + HALF_SQUARE);  
          Move_Head(U + HALF_SQUARE, Y + HALF_SQUARE);
        }
        else if(Coord_V - Coord_Y == -1)  {
          Move_Head(X - HALF_SQUARE, Y - HALF_SQUARE);
          Move_Head(U + HALF_SQUARE, Y - HALF_SQUARE);  
        }
        else {
          Move_Head(X - HALF_SQUARE, Y - HALF_SQUARE);
          Move_Head(X - HALF_SQUARE, V + HALF_SQUARE);  
        }        
      }     
                              
      Move_Head_Extra(U + X_CORNER_B, V + Y_CORNER_B);   
      Disengage_Head();       
      Update_Flag = 1;                         
    }

    // E) JUST AN ORDINARY MOVE THEN!
    else  { 
      Move_Head(X, Y);          
      Engage_Head();                              
      Move_Head_Extra(U + X_CORNER_B, V + Y_CORNER_B);   
      Disengage_Head();      
      Update_Flag = 1;           
    }

    if(Remove_Piece_Flag) {
      if(X_CORNER_B == 0) {
        Remove_Piece(Coord_U, Coord_V, -HALF_SQUARE, Y_CORNER_B);
      }
      else if(Y_CORNER_B == 0)  {
        Remove_Piece(Coord_U, Coord_V, X_CORNER_B, -HALF_SQUARE);
      }
      else  {
        Remove_Piece(Coord_U, Coord_V, X_CORNER_B, Y_CORNER_B);
      }
             
      Move_Head(U + X_CORNER_B, V + Y_CORNER_B);
      Engage_Head(); 
      Move_Head_Extra(U, V);
      Disengage_Head();
      Remove_Piece_Flag = 0;            
    }

    if(Update_Flag)
    {
      Update_Flag = 0;
      Update_Position_Table(Coord_X, Coord_Y, Coord_U, Coord_V);
    }

    while(Pause_Flag) {
      static unsigned long Toggle_Time = millis();
      if(millis() - Toggle_Time > 250)  {
        Toggle_Time = millis();
        digitalWrite(Mode_Button_Light, !digitalRead(Mode_Button_Light)); 
      }      
    }    

    Serial.print("D");   
    #ifdef _DEBUG 
      Print_Piece_Track_Buffer();
    #endif
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
    if(Command_buffer[0] == 'r')
      {        
        Setup_Pieces();
        Piece_Track_Buffer_Initalise();
        Zero_Head();
        Serial.print("D");
        return(0);

        #ifdef _DEBUG
          Serial.println();
          Print_Piece_Track_Buffer();
        #endif           
      }
    if(Command_buffer[0] == 'q')
      {                
        Piece_Track_Buffer_Initalise();
        Zero_Head();
        Serial.print("D");
        return(0);

        #ifdef _DEBUG
          Serial.println();
          Print_Piece_Track_Buffer();
        #endif           
      }      
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
void Setup_Pieces()
{
  #ifdef _DEBUG
    Serial.println("Setting up all the Pieces");
  #endif
  
  char Piece_to_find_Buffer[] = "RNBQKBNRPPPPPPPPpppppppprnbqkbnr";
  byte Piece_Locations_X = 1;
  byte Piece_Locations_Y = 1;
  byte Pieces_Found = 0;

  while(Pieces_Found < 32)  
  {    
    #ifdef _DEBUG
      delay(100);
      Serial.print("Piece to find: "); Serial.print(Piece_to_find_Buffer[Pieces_Found]);
      Serial.print("  Piece_Locations_X: "); Serial.print(Piece_Locations_X);
      Serial.print("  Piece_Locations_Y: "); Serial.print(Piece_Locations_Y);
      Serial.print("  Pieces found: "); Serial.println(Pieces_Found);
    #endif
    
    for(int j = 9; j >= 0; j--) {
      for(int i = 9; i >= 0; i--) {      
        if(Piece_Track_Buffer[i][j] == Piece_to_find_Buffer[Pieces_Found])
        {           
          #ifdef _DEBUG
            Serial.print("Found piece at: "); Serial.print(i);
            Serial.print(", "); Serial.println(j);
          #endif
          
          // Don't move a piece that's on the right square...          
          byte Piece_in_Correct_Position = 0;          
          if(Piece_Track_Buffer[i][j] == 'p' and i >= 1 and i <= 8 and j == 7) {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'r' and i == 8 and j == 8)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'r' and i == 1 and j == 8)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'n' and i == 7 and j == 8)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'n' and i == 2 and j == 8)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'b' and i == 6 and j == 8)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'b' and i == 3 and j == 8)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'q' and i == 4 and j == 8)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'k' and i == 5 and j == 8)  {Piece_in_Correct_Position = 1;}

          if(Piece_Track_Buffer[i][j] == 'P' and i >= 1 and i <= 8 and j == 2) {Piece_in_Correct_Position = 1;} 
          if(Piece_Track_Buffer[i][j] == 'R' and i == 8 and j == 1)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'R' and i == 1 and j == 1)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'N' and i == 7 and j == 1)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'N' and i == 2 and j == 1)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'B' and i == 6 and j == 1)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'B' and i == 3 and j == 1)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'Q' and i == 4 and j == 1)  {Piece_in_Correct_Position = 1;}
          if(Piece_Track_Buffer[i][j] == 'K' and i == 5 and j == 1)  {Piece_in_Correct_Position = 1;}
          
          if(Piece_in_Correct_Position and i == Piece_Locations_X and j == Piece_Locations_Y)
          {
            Pieces_Found++;            
            Piece_Locations_X++;

            #ifdef _DEBUG
              Serial.println("Piece in (i,j) location already correct");          
            #endif                       
          }    

          // Don't move a piece on to a square that has the correct piece on it...
          else if(Piece_Track_Buffer[i][j] == Piece_Track_Buffer[Piece_Locations_X][Piece_Locations_Y])
          {
            Pieces_Found++;            
            Piece_Locations_X++;
            Piece_in_Correct_Position = 1;

            #ifdef _DEBUG
              Serial.println("Piece in (X,Y) location already correct");            
            #endif            
          }                         
          
          if(!Piece_in_Correct_Position)
          {
            #ifdef _DEBUG
              Serial.println("Piece being moved");          
            #endif
                        
            if(Piece_Track_Buffer[Piece_Locations_X][Piece_Locations_Y] != '0') { 
              Remove_Piece(Piece_Locations_X, Piece_Locations_Y, HALF_SQUARE, HALF_SQUARE);                                           
            }          
            
            Move_Head(STEPS_PER_SQUARE * i, STEPS_PER_SQUARE * j);
            Engage_Head();
            
            long X_CORNER_A, Y_CORNER_A, X_CORNER_B, Y_CORNER_B;
            if(Piece_Locations_X >= i and Piece_Locations_Y >= j) {               // move up and right              
              X_CORNER_A = HALF_SQUARE;
              Y_CORNER_A = HALF_SQUARE;   
              X_CORNER_B = -HALF_SQUARE;
              Y_CORNER_B = -HALF_SQUARE;                          
            }
            else if(Piece_Locations_X >= i and Piece_Locations_Y <= j) {          // move down and right
              X_CORNER_A = HALF_SQUARE;
              Y_CORNER_A = -HALF_SQUARE;
              X_CORNER_B = -HALF_SQUARE;
              Y_CORNER_B = HALF_SQUARE;               
            }
            else if(Piece_Locations_X <= i and Piece_Locations_Y >= j) {          // move up and left
              X_CORNER_A = -HALF_SQUARE;
              Y_CORNER_A = HALF_SQUARE;
              X_CORNER_B = HALF_SQUARE;
              Y_CORNER_B = -HALF_SQUARE;                            
            }
            else  {                                                               // move down and left
              X_CORNER_A = -HALF_SQUARE;
              Y_CORNER_A = -HALF_SQUARE;
              X_CORNER_B = HALF_SQUARE;
              Y_CORNER_B = HALF_SQUARE;                            
            }
            
            Move_Head((STEPS_PER_SQUARE * i) + X_CORNER_A, (STEPS_PER_SQUARE * j) + Y_CORNER_A);            
            Move_Head((STEPS_PER_SQUARE * Piece_Locations_X) + X_CORNER_B, (STEPS_PER_SQUARE * j) + Y_CORNER_A);            
            Move_Head((STEPS_PER_SQUARE * Piece_Locations_X) + X_CORNER_B, (STEPS_PER_SQUARE * Piece_Locations_Y) + Y_CORNER_B);            
            Move_Head_Extra((STEPS_PER_SQUARE * Piece_Locations_X), (STEPS_PER_SQUARE * Piece_Locations_Y));
            
            Disengage_Head();
            
            Update_Position_Table(i, j, Piece_Locations_X, Piece_Locations_Y);           
  
            Pieces_Found++;            
            Piece_Locations_X++;            
          }

          if(Piece_Locations_X == 9)  {
            Piece_Locations_X = 1;
            Piece_Locations_Y++;
          }
          if(Piece_Locations_Y == 3)  {
            Piece_Locations_Y = 7;   
          }        
        }                
      }      
    }
  }

  #ifdef _DEBUG
    Serial.println("Setup complete");
  #endif
  
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Print_Piece_Track_Buffer()
{
  Serial.println("Piece Track Buffer: ");
  
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

  A = constrain(A, STEPS_MIN, STEPS_MAX + X_OFFSET);                              // Constrain range for symmetry
  B = constrain(B, STEPS_MIN, STEPS_MAX + Y_OFFSET);   
  
  A = constrain(A, STEPS_MIN, STEPS_MAX);                                         // Constrain range for safety
  B = constrain(B, STEPS_MIN, STEPS_MAX);  
        
  Xstepper.moveTo(A);                                                             // Set the postion to move to
  Ystepper.moveTo(B);

  if(Head_Enabled_Flag) {                                                         // Move the head faster if it's not dragging a piece around
    Xstepper.setMaxSpeed(STEPPER_ENGAGED_SPEED);
    Xstepper.setAcceleration(STEPPER_ACCELERATION);   
    Ystepper.setMaxSpeed(STEPPER_ENGAGED_SPEED);
    Ystepper.setAcceleration(STEPPER_ACCELERATION);
  }
  else  {
    Xstepper.setMaxSpeed(STEPPER_DISENGAGED_SPEED);
    Xstepper.setAcceleration(STEPPER_ACCELERATION);   
    Ystepper.setMaxSpeed(STEPPER_DISENGAGED_SPEED);
    Ystepper.setAcceleration(STEPPER_ACCELERATION);    
  }  

  if(Xstepper.distanceToGo()) {                                                   // Enable steppers only if they need enabling
    digitalWrite(X_Stepper_nEn, LOW);
  } 
  if(Ystepper.distanceToGo()) {                                               
    digitalWrite(Y_Stepper_nEn, LOW);
  }
  delay(5);
  
  while(Xstepper.distanceToGo() or Ystepper.distanceToGo()) {                     // Loop until the head has reached the desired position.
    Check_if_Paused();
    Xstepper.run();  
    Ystepper.run();
  }

  digitalWrite(X_Stepper_nEn, HIGH);                                                // Disable steppers
  digitalWrite(Y_Stepper_nEn, HIGH);  
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Move_Head_Extra(long A, long B)                                                // Function to move head a little further in the same direction to account for
{                                                                                   // the chess piece following slightly behind due to friction etc
  A += X_OFFSET;
  B += Y_OFFSET;
  
  long Initial_X = Xstepper.currentPosition();
  long Initial_Y = Ystepper.currentPosition();     

  long X_Diff = A - Initial_X;
  long Y_Diff = B - Initial_Y;

  long Mag_Diff = (long)sqrt((X_Diff*X_Diff) + (Y_Diff*Y_Diff));
  A += (X_Diff * STEPS_EXTRA) / Mag_Diff;
  B += (Y_Diff * STEPS_EXTRA) / Mag_Diff;

  A = constrain(A, STEPS_MIN, STEPS_MAX + X_OFFSET);                              // Constrain range for symmetry
  B = constrain(B, STEPS_MIN, STEPS_MAX + Y_OFFSET);
  
  A = constrain(A, STEPS_MIN, STEPS_MAX);                                         // Constrain range for safety
  B = constrain(B, STEPS_MIN, STEPS_MAX);  
        
  Xstepper.moveTo(A);
  Ystepper.moveTo(B);

  if(Head_Enabled_Flag) {
    Xstepper.setMaxSpeed(STEPPER_ENGAGED_SPEED);
    Xstepper.setAcceleration(STEPPER_ACCELERATION);   
    Ystepper.setMaxSpeed(STEPPER_ENGAGED_SPEED);
    Ystepper.setAcceleration(STEPPER_ACCELERATION);
  }
  else  {
    Xstepper.setMaxSpeed(STEPPER_DISENGAGED_SPEED);
    Xstepper.setAcceleration(STEPPER_ACCELERATION);   
    Ystepper.setMaxSpeed(STEPPER_DISENGAGED_SPEED);
    Ystepper.setAcceleration(STEPPER_ACCELERATION);    
  } 

  if(Xstepper.distanceToGo()) {                                                   // Enable steppers if they need enabling
    digitalWrite(X_Stepper_nEn, LOW);
  } 
  if(Ystepper.distanceToGo()) {                                               
    digitalWrite(Y_Stepper_nEn, LOW);
  }
  delay(5);
  
  while(Xstepper.distanceToGo() or Ystepper.distanceToGo()) {                     // Loop until the head has reached the desired position
    Check_if_Paused();
    Xstepper.run();  
    Ystepper.run();     
  }

  digitalWrite(X_Stepper_nEn, HIGH);                                                // Disable steppers
  digitalWrite(Y_Stepper_nEn, HIGH);
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Check_if_Paused()
{
  if(!Pause_Flag)                                                               
    return;

  digitalWrite(X_Stepper_nEn, HIGH);                                                // Disable steppers
  digitalWrite(Y_Stepper_nEn, HIGH);

  while(Pause_Flag) {
    static unsigned long Toggle_Time = millis();
    if(millis() - Toggle_Time > 250)  {
      Toggle_Time = millis();
      digitalWrite(Mode_Button_Light, !digitalRead(Mode_Button_Light)); 
    }
  }

  digitalWrite(Mode_Button_Light, HIGH);                                          // Ensure button light is on 

  if(Xstepper.distanceToGo()) {                                                   // Enable steppers if they need enabling before returning
    digitalWrite(X_Stepper_nEn, LOW);
  } 
  if(Ystepper.distanceToGo()) {                                               
    digitalWrite(Y_Stepper_nEn, LOW);
  }
  delay(5);
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Engage_Head()
{  
  servo1.write(70); 
  Head_Enabled_Flag = 1; 
  delay(500);
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Disengage_Head()
{  
  servo1.write(175); 
  Head_Enabled_Flag = 0;
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
void Remove_Piece(byte Coord_U, byte Coord_V, long X_CORNER_B, long Y_CORNER_B)
{
  // Coord_U, Coord_V hold the position of the piece to remove. Coord_W, Coord_Z hold position to take piece to
  // 1) What colour is the piece to remove, 2) find next available square on sidelines, 3) perform movements to get piece there, 4) Update position table
  
  int Taken_Piece, Coord_W, Coord_Z;    
  
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
    Coord_W = 0;
    Coord_Z = 9;
    
    while(Coord_Z >= 0)                                                           // Search from top left down to bottom left for a free square
    {
      if(Piece_Track_Buffer[0][Coord_Z] == '0') {
        break;
      }
      Coord_Z--;    
    }

    if(Coord_Z < 0) {                                                             // If there weren't any free squares on the left, search from bottom left to bottom right
      Coord_Z = 0;
      while(Coord_W <= 9)                                                                 
      {
        if(Piece_Track_Buffer[Coord_W][0] == '0') {
          break;
        }
        Coord_W++;    
      }
    }

    if(Coord_W == 10) {                                                           // By the time we reach here, we should have the x,y coordinates of the next free slot on white's sidelines
      #ifdef _DEBUG
        Serial.println("There's been a problem searching for a free spot on white's sidelines!");
      #endif
      return;
    }
  }
  
  else  
  {
    Coord_W = 9;
    Coord_Z = 0;
    
    while(Coord_Z <= 9)                                                           // Search from bottom right up to top right for a free square
    {
      if(Piece_Track_Buffer[9][Coord_Z] == '0') {
        break;
      }
      Coord_Z++;    
    }

    if(Coord_Z > 9) {                                                             // If there weren't any free squares on the right, search from top right to top left
      Coord_Z = 9;
      while(Coord_W >= 0)                                                                 
      {
        if(Piece_Track_Buffer[Coord_W][9] == '0') {
          break;
        }
        Coord_W--;    
      }
    }

    if(Coord_W < 0) {                                                             // By the time we reach here, we should have the x,y coordinates of the next free slot on black's sidelines
      #ifdef _DEBUG
        Serial.println("There's been a problem searching for a free spot on black's sidelines!");
      #endif
      return;
    }    
  }

  #ifdef _DEBUG
    Serial.print("Position to place removed piece is: (");
    Serial.print(Coord_W); Serial.print(", "); Serial.print(Coord_Z); Serial.println(")");
  #endif

  // 3)  --------------- Perform the movements to get the piece there ------------
  long X = Coord_U * STEPS_PER_SQUARE;                                            // Initial square
  long Y = Coord_V * STEPS_PER_SQUARE;
    
  long U = Coord_W * STEPS_PER_SQUARE;                                            // Final square
  long V = Coord_Z * STEPS_PER_SQUARE;

  if(Coord_W == 0)                                                                // Left sideline
  {      
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X, Y - Y_CORNER_B);                                                 // Move to (X, Y+0.5)          
    Move_Head(HALF_SQUARE, Y - Y_CORNER_B);                                       // Move to (0.5, Y+0.5):         
    Move_Head(HALF_SQUARE, V);                                                    // Move to (0.5, V)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();      
  }
  else if(Coord_W == 9)                                                           // Right sideline
  {
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X, Y - Y_CORNER_B);                                                 // Move to (X, Y+0.5)          
    Move_Head(((17 * STEPS_PER_SQUARE)/2), Y - Y_CORNER_B);                       // Move to (8.5, Y+0.5):         
    Move_Head(((17 * STEPS_PER_SQUARE)/2), V);                                    // Move to (8.5, V)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();    
  }
  else if(Coord_Z == 0)                                                           // Bottom sideline
  {
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X - X_CORNER_B, Y);                                                 // Move to (X+0.5, Y)          
    Move_Head(X - X_CORNER_B, HALF_SQUARE);                                       // Move to (X+0.5, 0.5):         
    Move_Head(U, HALF_SQUARE);                                                    // Move to (U, 0.5)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();    
  }
  else if(Coord_Z == 9)                                                           // Top sideline
  {
    Move_Head(X, Y);                                                              // Move to (X,Y)    
    Engage_Head();                                                                // Engage Head
    Move_Head(X - X_CORNER_B, Y);                                                 // Move to (X+0.5, Y)          
    Move_Head(X - X_CORNER_B, ((17 * STEPS_PER_SQUARE)/2));                       // Move to (X+0.5, 8.5):         
    Move_Head(U, ((17 * STEPS_PER_SQUARE)/2));                                    // Move to (U, 8.5)                 
    Move_Head_Extra(U, V);                                                        // Move to (U, V)  
    Disengage_Head();    
  }

  // 4)) Update position table
  Update_Position_Table(Coord_U, Coord_V, Coord_W, Coord_Z);
}

//--------------------------------------------------------------------------------
//################################################################################
//--------------------------------------------------------------------------------
void Zero_Head()
{
  #ifdef _DEBUG                            
    Serial.println("Zeroing Head");
  #endif

  Xstepper.setMaxSpeed(STEPPER_ZEROING_SPEED);
  Xstepper.setAcceleration(STEPPER_ACCELERATION);    
  Ystepper.setMaxSpeed(STEPPER_ZEROING_SPEED);
  Ystepper.setAcceleration(STEPPER_ACCELERATION);

  // --------------- First move out of corner if currently in it:
  Xstepper.move(STEPS_MAX);
  digitalWrite(X_Stepper_nEn, LOW);                                                 // Enable stepper 
  while(!digitalRead(X_Limit_Switch))
  { 
    Xstepper.run();             
  }
  digitalWrite(X_Stepper_nEn, HIGH);                                                // Disable stepper
  delay(250);

  Ystepper.move(STEPS_MAX);
  digitalWrite(Y_Stepper_nEn, LOW);                                                 // Enable stepper
  while(!digitalRead(Y_Limit_Switch))
  { 
    Ystepper.run();     
  }
  digitalWrite(Y_Stepper_nEn, HIGH);                                                // Disable stepper
  delay(250);

  // --------------- Now move into corner:
  Xstepper.move(-STEPS_MAX);
  digitalWrite(X_Stepper_nEn, LOW);                                                 // Enable stepper 
  while(digitalRead(X_Limit_Switch))
  { 
    Xstepper.run();             
  }  
  digitalWrite(X_Stepper_nEn, HIGH);                                                // Disable stepper
  delay(250);

  Ystepper.move(-STEPS_MAX);
  digitalWrite(Y_Stepper_nEn, LOW);                                                 // Enable stepper
  while(digitalRead(Y_Limit_Switch))
  { 
    Ystepper.run();             
  } 
  digitalWrite(Y_Stepper_nEn, HIGH);                                                // Disable stepper
  delay(250);

  Xstepper.setCurrentPosition(X_ZERO_OFFSET); 
  Ystepper.setCurrentPosition(Y_ZERO_OFFSET);
  Move_Head(0,0);

  #ifdef _DEBUG                            
    Serial.println("Head in zero position");    
  #endif
}

//--------------------------------------------------------------------------------
//############### INTERRUPT FUNCTIONS ############################################
//--------------------------------------------------------------------------------
void Button_Press()
{
  //Debounce Button:
  static unsigned long last_interrupt_time = 0;  
  unsigned long interrupt_time = millis();
  
  // If interrupts come faster than 500ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 500) 
  {
    Pause_Flag = !Pause_Flag;        
  }
    
  last_interrupt_time = interrupt_time;
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
















