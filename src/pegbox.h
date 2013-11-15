/***********************************************************************
 *
 * Copyright (C) 2013 Omid Nikta <omidnikta@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#ifndef PEGBOX_H
#define PEGBOX_H
#include "emptybox.h"

enum PEG_STATE {
	PEG_INITIAL		= 0,
	PEG_FILLED		= 1,
	PEG_EMPTY		= 2,
	PEG_DRAGED		= 3
};

#include <QGraphicsItem>

class Peg;
class QGraphicsEllipseItem;
class Board;

class PegBox : public EmptyBox
{
public:
	PegBox(const QPoint& position, Board* board, QGraphicsItem* parent = 0);
	bool hasPeg() const {return mPeg != 0;}
	Peg* getPeg() const {return mPeg;}
	void setPeg(Peg* peg);
	void setBoard(Board* board) {mBoard = board;}
	void setPegState(const PEG_STATE& state = PEG_EMPTY);
	void setPegColor(int color_number);
	int getPegColor();
	void setBoxState(const BOX_STATE& state = BOX_FUTURE);
	bool isPegVisible();


	PEG_STATE getPegState() const {return mPegState;}

private:
	PEG_STATE mPegState;
	QGraphicsEllipseItem* mCircle;
	Board* mBoard;
	Peg* mPeg;
};

#endif // PEGBOX_H
