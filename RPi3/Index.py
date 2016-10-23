# Electromechanical ChessBoard Index.py
# James Fotherby - October 2016
#
# This is the main file for the project and it calls in all other libraries:
#	-	pystockfish - A python interface to the Stockfish chess engine
#	- 	ChessBoard  - A Chess manager
#	
#	
# It's the job of the Arduino to receive move commands and carry them out. It's also the job of the
# Arduino to set the board up to a given specification. All the Pi does is give move commands.
# 
# The user's move is validated and any reasons for it not being valid are explained. Whether the game
# is won or not is checked, then the computer makes a move and the cycle repeats.
#
#
# Demo mode / voice control mode.
#
# Voice recognition will be an important part of this project. 
#
#

from pystockfish import *
from ChessBoard import ChessBoard
import serial
import time

def bmovelisttostr(moves):
        movestr = ''
        for h in moves:
            movestr += h + ' '
        return movestr.strip()
		
		
#----------------------------- Main --------------------------------
engine = Engine(depth=5)												# Instance of the chess engine object
chessboard = ChessBoard()												# Instance of the ChessBoard manager

engine.newgame()
chessboard.resetBoard()													# Resets the chess board and all states
engine.setposition([])													# Inform the engine of the positions

ser = serial.Serial('/dev/ttyACM0', 115200)								# Opens a serial port

time.sleep(5)
ToTx = 'mx9x9' 
ser.write(ToTx.encode())
time.sleep(5)

while not chessboard.isGameOver():
	movedict = engine.bestmove()										# Get the engine to choose a move	
	bestmove = movedict.get('move')										# Discover what that move was
	info = movedict.get('info')
	ponder = movedict.get('ponder')	
		
	chessboard.addTextMove(bestmove)									# Add the CE's move to the brd manager
	ToTx = 'm' + chessboard.getLastTextMove(ChessBoard.AN)				# Prepare command for sending to Arduino
	ser.write(ToTx.encode())											# Output CE's move to Arduino		
#----------------------------------------------------------------------- 	
	
	chessboard.printBoard()												# Outputs the board to the terminal

#----------------------------------------------------------------------- 	
	while True:															# Get the user's move and validate it
		move = input("Enter move: ")									
		if(chessboard.addTextMove(move)):			
			break
		print("That's an invalid move!")
		ToTx = 'mx9x9' 
		ser.write(ToTx.encode())
		print(chessboard.getReason())	
	
	engine.setposition(chessboard.getAllTextMoves(ChessBoard.AN))		# Inform the engine of the new position
	
	ToTx = 'm' + chessboard.getLastTextMove(ChessBoard.AN)				# Prepare command for sending to Arduino
	ser.write(ToTx.encode())											# Output user's move to Arduino
	
	print("Chess Engine's turn...")
	time.sleep(15)	
	
print("Game Over!")
	
	
	
	
	
	
	
	