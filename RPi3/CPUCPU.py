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
engineB = Engine(depth=10)

chessboard = ChessBoard()												# Instance of the ChessBoard manager

engine.newgame()
engineB.newgame()

chessboard.resetBoard()													# Resets the chess board and all states

engine.setposition([])													# Inform the engine of the positions
engineB.setposition([])

ser = serial.Serial('/dev/ttyACM0', 115200)								# Opens a serial port

time.sleep(2)
ToTx = 'mx9x9' 
ser.write(ToTx.encode())
time.sleep(2)

for i in range(0, 25):
	movedict = engine.bestmove()										# Get the engine to choose a move	
	bestmove = movedict.get('move')										# Discover what that move was
	info = movedict.get('info')
	ponder = movedict.get('ponder')	
		
	chessboard.addTextMove(bestmove)									# Add the CE's move to the brd manager
	ToTx = 'm' + chessboard.getLastTextMove(ChessBoard.AN)				# Prepare command for sending to Arduino
	print(ToTx)
	ser.write(ToTx.encode())											# Output CE's move to Arduino	
	
#----------------------------------------------------------------------- 	

	time.sleep(1)
	Rx = ser.read(1)
	print(Rx.decode())	
	print("Chess Engine B's turn...")
	chessboard.printBoard()												# Outputs the board to the terminal

#----------------------------------------------------------------------- 	

	engineB.setposition(chessboard.getAllTextMoves(ChessBoard.AN))		# Inform the engine of the new position

	movedict = engineB.bestmove()										# Get the engine B to choose a move	
	bestmove = movedict.get('move')										# Discover what that move was
	info = movedict.get('info')
	ponder = movedict.get('ponder')	
		
	chessboard.addTextMove(bestmove)									# Add the CE's move to the brd manager
	ToTx = 'm' + chessboard.getLastTextMove(ChessBoard.AN)				# Prepare command for sending to Arduino
	print(ToTx)
	ser.write(ToTx.encode())											# Output CE's move to Arduino	
	
#----------------------------------------------------------------------- 	
	
	time.sleep(1)
	Rx = ser.read(1)
	print(Rx.decode())	
	print("Chess Engine A's turn...")
	chessboard.printBoard()												# Outputs the board to the terminal

#-----------------------------------------------------------------------

	engine.setposition(chessboard.getAllTextMoves(ChessBoard.AN))		# Inform the engine of the new position
	
print("Game Over!")

time.sleep(2)
ToTx = 'mx9x9' 
ser.write(ToTx.encode())
time.sleep(10)
	
	
	
	
	
	
	
	