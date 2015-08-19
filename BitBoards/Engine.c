#include <stdio.h>
#include <stdlib.h>

#include "Types.h"
#include "Board.h"
#include "BitTools.h"
#include "Engine.h"
#include "Moves.h"

Move ** getAllMoves(Board * board, int * index, int turn){
	Move ** moves = malloc(sizeof(Move * ) * 256);
	
	getKingMoves(board, moves, index, turn);
	getQueenMoves(board, moves, index, turn);
	getRookMoves(board, moves, index, turn);
	getKnightMoves(board, moves, index, turn);
	getBishopMoves(board, moves, index, turn);
	getPawnMoves(board, moves, index, turn);
	
	return validateCheck(board,index,moves,turn);
}

void getKnightMoves(Board * board, Move ** moves, int * index, int turn){
	BitBoard friendly = board->Colors[turn];
	BitBoard enemy = board->Colors[!turn];
	
	BitBoard friendlyKnights = board->Pieces[KNIGHT] & friendly;
	while(friendlyKnights != 0){
		int bit = getLSB(friendlyKnights);
		BitBoard attackable = board->KnightMap[bit] & ~friendly;
		
		while (attackable != 0){
			int lsb = getLSB(attackable);
			Move * move = malloc(sizeof(Move));
			move->Start = bit;
			move->End = lsb;
			move->CapturedType = getSquare(lsb,board);
			move->MovedType = KNIGHT;
			move->Type = NormalMove;
			moves[(*index)++] = move;
			attackable ^= 1ull << lsb;
		}
		
		friendlyKnights ^= 1ull << bit; 	
	}
}

void getKingMoves(Board * board, Move ** moves, int * index, int turn){
	BitBoard friendly = board->Colors[turn];
	BitBoard enemy = board->Colors[!turn];
	
	if (board->Pieces[KING] & friendly == 0)
		return;
	int bit = getLSB((board->Pieces[KING] & friendly));
	BitBoard attackable = board->KingMap[bit] & ~friendly;
	
	while (attackable != 0){
		int lsb = getLSB(attackable);
		Move * move = malloc(sizeof(Move));
		move->Start = bit;
		move->End = lsb;
		move->CapturedType = getSquare(lsb,board);
		move->MovedType = KING;
		
		if(board->ValidCastles[turn][0]){
			if(board->ValidCastles[turn][1])
				move->Type = BreaksBothCastlesMove;
			else
				move->Type = BreaksLeftCastleMove;
		} else if(board->ValidCastles[turn][1]){
			move->Type = BreaksRightCastleMove;
		} else{
			move->Type = NormalMove;
		}
		
		moves[(*index)++] = move;
		attackable ^= 1ull << lsb;
	}
	
	
	if (board->Castled[turn] == 0){
		if (board->ValidCastles[turn][0] == 1){
			if ((board->Colors[0] | board->Colors[1]) & LeftCastleBoards[turn] == 0){
				Move * move = malloc(sizeof(Move));				
				move->Type = LeftCastleMove;
				moves[(*index)++] = move;
			}
		}
		if (board->ValidCastles[turn][1] == 1){
			if ((board->Colors[0] | board->Colors[1]) & RightCastleBoards[turn] == 0){
				Move * move = malloc(sizeof(Move));				
				move->Type = RightCastleMove;
				moves[(*index)++] = move;
			}
		}
	}
}

void getPawnMoves(Board * board, Move ** moves, int * index, int turn){

	int i, forwardShift, leftShift, rightShift;
	BitBoard friendly = board->Colors[turn];
	BitBoard enemy = board->Colors[!turn];
	BitBoard pawns, oneMove, onePromote, twoMove;
	BitBoard leftMove, leftPromote, rightMove, rightPromote;
	
	if (turn == WHITE){
		forwardShift = 8;
		leftShift = 7;
		rightShift = 9;
	}
	else if (turn == BLACK){
		forwardShift = -8;
		leftShift = -7;
		rightShift = -9;
	}
	
	pawns = board->Pieces[PAWN] & friendly;
	int types[4] = {
		PromoteBishopMove, PromoteKnightMove,
		PromoteRookMove, PromoteQueenMove
	};
	
	if (turn == WHITE){
		oneMove = (pawns << 8) & ~(friendly | enemy);
		onePromote = oneMove & RANK_8;
		oneMove &= (~RANK_8);
		
		twoMove = ((oneMove & RANK_3) << 8) & ~(friendly | enemy);
		
		leftMove = ((pawns << 7) & ~(FILE_A)) & enemy;
		leftPromote = leftMove & RANK_8;
		leftMove &= ~RANK_8;
		
		rightMove = ((pawns << 9) & ~(FILE_H)) & enemy;
		rightPromote = rightMove & RANK_8;
		rightMove &= ~RANK_8; 
	}
	
	else if (turn == BLACK){
		oneMove = (pawns >> 8) & ~(friendly | enemy);
		onePromote = oneMove & RANK_1;
		oneMove &= (~RANK_1);
		
		twoMove = ((oneMove & RANK_6) >> 8) & ~(friendly | enemy);
		
		leftMove = ((pawns >> 7) & ~(FILE_H)) & enemy;
		leftPromote = leftMove & RANK_1;
		leftMove &= ~RANK_1;
		
		rightMove = ((pawns >> 9) & ~(FILE_A)) & enemy;
		rightPromote = rightMove & RANK_1;
		rightMove &= ~RANK_1;
	}
	
	while (oneMove != 0){
		int lsb = getLSB(oneMove);
		Move * move = malloc(sizeof(Move));
		move->Start = lsb - forwardShift;
		move->End = lsb;
		move->CapturedType = EMPTY;
		move->MovedType = PAWN;
		move->Type = NormalMove;
		moves[(*index)++] = move;
		oneMove ^= 1ull << lsb;
	}
	/*
	while (onePromote != 0){
		int lsb = getLSB(onePromote);
		for(i = 0; i < 4; i++){
			Move * move = malloc(sizeof(Move));
			move->Start = lsb - forwardShift;
			move->End = lsb;
			move->CapturedType = EMPTY;
			move->MovedType = PAWN;
			move->Type = types[i];
			moves[(*index)++] = move;
		}
		onePromote ^= 1ull << lsb;
	}*/
	
	while (twoMove != 0){
		int lsb = getLSB(twoMove);
		Move * move = malloc(sizeof(Move));
		move->Start = lsb - (2 * forwardShift);
		move->End = lsb;
		move->CapturedType = EMPTY;
		move->MovedType = PAWN;
		move->Type = PawnDoublePushMove;
		moves[(*index)++] = move;
		twoMove ^= 1ull << lsb;
	}
	
	while (leftMove != 0){
		int lsb = getLSB(leftMove);
		Move * move = malloc(sizeof(Move));
		move->Start = lsb - leftShift;
		move->End = lsb;
		move->CapturedType = getSquare(lsb,board);
		move->MovedType = PAWN;
		move->Type = NormalMove;
		moves[(*index)++] = move;
		leftMove ^= 1ull << lsb;
	}
	/*
	while (leftPromote != 0){
		int lsb = getLSB(leftPromote);
		for(i = 0; i < 4; i++){
			Move * move = malloc(sizeof(Move));
			move->Start = lsb - forwardShift;
			move->End = lsb;
			move->CapturedType = EMPTY;
			move->MovedType = PAWN;
			move->Type = types[i];
			moves[(*index)++] = move;
		}
		leftPromote ^= 1ull << lsb;
	}*/
	
	while (rightMove != 0){
		int lsb = getLSB(rightMove);
		Move * move = malloc(sizeof(Move));
		move->Start = lsb - rightShift;
		move->End = lsb;
		move->CapturedType = getSquare(lsb,board);
		move->MovedType = PAWN;
		move->Type = NormalMove;
		moves[(*index)++] = move;
		rightMove ^= 1ull << lsb;
	}
	/*
	while (rightPromote != 0){
		int lsb = getLSB(rightPromote);
		for(i = 0; i < 4; i++){
			Move * move = malloc(sizeof(Move));
			move->Start = lsb - forwardShift;
			move->End = lsb;
			move->CapturedType = EMPTY;
			move->MovedType = PAWN;
			move->Type = types[i];
			moves[(*index)++] = move;
		}
		rightPromote ^= 1ull << lsb;
	}*/
}

void getRookMoves(Board * board, Move ** moves, int * index, int turn){

	BitBoard friendly = board->Colors[turn];
	BitBoard enemy = board->Colors[!turn];
	
	BitBoard friendlyRooks = board->Pieces[ROOK] & friendly;
	while(friendlyRooks != 0){
		int bit = getLSB(friendlyRooks);
		
		BitBoard bbBlockers = (board->Colors[0] | board->Colors[1]) & board->OccupancyMaskRook[bit];
		int dbIndex = (int)((bbBlockers * board->MagicNumberRook[bit]) >> board->MagicShiftsRook[bit]);
		BitBoard attackable = board->MoveDatabaseRook[bit][dbIndex] & ~friendly;
		
		while (attackable != 0){
			int lsb = getLSB(attackable);
			Move * move = malloc(sizeof(Move));
			move->Start = bit;
			move->End = lsb;
			move->CapturedType = getSquare(lsb,board);
			move->MovedType = ROOK;
			
			if(board->ValidCastles[turn][0] && (bit ==  0 || bit == 56))
				move->Type = BreaksLeftCastleMove;
			else if(board->ValidCastles[turn][1] && (bit == 7 || bit == 63))
				move->Type = BreaksRightCastleMove;
			else
				move->Type = NormalMove;
				
			moves[(*index)++] = move;
			attackable ^= 1ull << lsb;
		}
		
		friendlyRooks ^= 1ull << bit;
	}
}

void getBishopMoves(Board * board, Move ** moves, int * index, int turn){

	BitBoard friendly = board->Colors[turn];
	BitBoard enemy = board->Colors[!turn];
	
	BitBoard friendlyBishops = board->Pieces[BISHOP] & friendly;
	while(friendlyBishops != 0){
		int bit = getLSB(friendlyBishops);
		
		BitBoard bbBlockers = (board->Colors[0] | board->Colors[1]) & board->OccupancyMaskBishop[bit];
		int dbIndex = (int)((bbBlockers * board->MagicNumberBishop[bit]) >> board->MagicShiftsBishop[bit]);
		BitBoard attackable = board->MoveDatabaseBishop[bit][dbIndex] & ~friendly;
		
		while (attackable != 0){
			int lsb = getLSB(attackable);
			Move * move = malloc(sizeof(Move));
			move->Start = bit;
			move->End = lsb;
			move->CapturedType = getSquare(lsb,board);
			move->MovedType = BISHOP;
			move->Type = NormalMove;
			moves[(*index)++] = move;
			attackable ^= 1ull << lsb;
		}
		
		friendlyBishops ^= 1ull << bit;
	}
}

void getQueenMoves(Board * board, Move ** moves, int * index, int turn){

	BitBoard friendly = board->Colors[turn];
	BitBoard enemy = board->Colors[!turn];
	
	BitBoard friendlyQueens = board->Pieces[QUEEN] & friendly;
	while(friendlyQueens != 0){
		int bit = getLSB(friendlyQueens);
		
		BitBoard bbBlockers = (board->Colors[0] | board->Colors[1]) & board->OccupancyMaskRook[bit];
		int dbIndex = (int)((bbBlockers * board->MagicNumberRook[bit]) >> board->MagicShiftsRook[bit]);
		BitBoard attackable = board->MoveDatabaseRook[bit][dbIndex] & ~friendly;
		
		while (attackable != 0){
			int lsb = getLSB(attackable);
			Move * move = malloc(sizeof(Move));
			move->Start = bit;
			move->End = lsb;
			move->CapturedType = getSquare(lsb,board);
			move->MovedType = QUEEN;
			move->Type = NormalMove;
			moves[(*index)++] = move;
			attackable ^= 1ull << lsb;
		}
		
		bbBlockers = (board->Colors[0] | board->Colors[1]) & board->OccupancyMaskBishop[bit];
		dbIndex = (int)((bbBlockers * board->MagicNumberBishop[bit]) >> board->MagicShiftsBishop[bit]);
		attackable = board->MoveDatabaseBishop[bit][dbIndex] & ~friendly;
		
		while (attackable != 0){
			int lsb = getLSB(attackable);
			Move * move = malloc(sizeof(Move));
			move->Start = bit;
			move->End = lsb;
			move->CapturedType = getSquare(lsb,board);
			move->MovedType = QUEEN;
			move->Type = NormalMove;
			moves[(*index)++] = move;
			attackable ^= 1ull << lsb;
		}
		
		friendlyQueens ^= 1ull << bit;
	}
}

Move ** validateCheck(Board * board, int * index, Move ** moves, int turn){
	
	Move ** fixed = malloc(sizeof(Move *) * 256);
	int size = *index;
	
	*index = 0;
	
	int i;
	for(i = 0; i < size; i++){
		ApplyMove(board,moves[i],turn);
		int valid = validateMove(board,turn);
		RevertMove(board,moves[i],turn);
		
		if(valid == 1)
			fixed[(*index)++] = moves[i];
		else
			free(moves[i]);
		
	}
	
	free(moves);
	return fixed;
}

int validateMove(Board * board, int turn){
	BitBoard friendly = board->Colors[turn];
	BitBoard enemy = board->Colors[!turn];
	BitBoard all = friendly | enemy;
	BitBoard king = friendly & board->Pieces[KING];
	BitBoard bbBlockers, attackable;
	
	int dbIndex;
	int kingSquare = getLSB(king);
	
	//Bishop Check
	bbBlockers = all & board->OccupancyMaskBishop[kingSquare];
	dbIndex = (int)((bbBlockers * board->MagicNumberBishop[kingSquare]) >> board->MagicShiftsBishop[kingSquare]);
	attackable = board->MoveDatabaseBishop[kingSquare][dbIndex] & ~friendly;
	if (((board->Pieces[BISHOP] | board->Pieces[QUEEN]) & enemy & attackable) != 0)
		return 0;
	
	// Rook Check
	bbBlockers = all & board->OccupancyMaskRook[kingSquare];
	dbIndex = (int)((bbBlockers * board->MagicNumberRook[kingSquare]) >> board->MagicShiftsRook[kingSquare]);
	attackable = board->MoveDatabaseRook[kingSquare][dbIndex];
	if (((board->Pieces[ROOK] | board->Pieces[QUEEN]) & enemy & attackable) != 0)
		return 0;
		
	// Knight Check
	if (((board->KnightMap[kingSquare]) & (enemy & board->Pieces[KNIGHT])) != 0)
		return 0;
	
	// Pawn Check
	BitBoard enemyPawns = enemy & board->Pieces[PAWN];
	if (turn == WHITE){
		if ((((king << 7) & ~FILE_A) & enemyPawns) != 0)
			return 0;
		if ((((king << 9) & ~FILE_H) & enemyPawns) != 0)
			return 0;
	}else if (turn == BLACK){
		if ((((king >> 7) & ~FILE_H) & enemyPawns) != 0)
			return 0;
		if ((((king >> 9) & ~FILE_A) & enemyPawns) != 0)
			return 0;
	}
	
	// King Check
	if (((board->KingMap[kingSquare]) & (enemy & board->Pieces[KING])) != 0)
		return 0;
		
	return 1;
}

