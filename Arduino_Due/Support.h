#ifndef _SUPPORT_H_
#define _SUPPORT_H_


//--- Defines: -------------------------------------------------------------------


//--- Routines: ------------------------------------------------------------------
void Configure_Pins();                                                   
void Piece_Track_Buffer_Initalise();
byte Get_Command();
void Remove_Piece(byte X, byte Y);
void Move_Head(long A, long B);
void Move_Head_Extra(long A, long B);
void Engage_Head();
void Disengage_Head(); 
void Update_Position_Table(int Coord_X, int Coord_Y, int Coord_U, int Coord_V);

//--- Classes --------------------------------------------------------------------


#endif 





