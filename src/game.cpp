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

#include "game.h"
#include "rules.h"
#include "board.h"
#include "button.h"
#include "pegbox.h"
#include "pinbox.h"
#include "solver.h"
#include "message.h"
#include "ctime"

inline static void setStateOfList(QList<PegBox *> *boxlist, const Box::State &state_t)
{
	foreach (PegBox *box, *boxlist)
		box->setState(state_t);
}

Game::Game(Rules *game_rules, Board *board_aid, QWidget *parent):
	QGraphicsView(parent),
	state(State::None),
	rules(game_rules),
	board(board_aid),
	solver(0)
{
	auto scene = new QGraphicsScene(this);
	setScene(scene);
	scene->setSceneRect(0, 0, 320, 560);
	fitInView(sceneRect(), Qt::KeepAspectRatio);
	setCacheMode(QGraphicsView::CacheNone);
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	setFrameStyle(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

Game::~Game()
{
	if (solver) {
		emit interuptSignal();
		solver->quit();
		solver->wait();
		solver->deleteLater();
	}

	scene()->clear();
}

void Game::codeRowFilled(const bool &m_filled)
{
	if(rules->mode == Rules::Mode::HVM) {
		if (m_filled) {
			okButton->setVisible(true);
			okButton->setEnabled(true);
			state = State::WaittingPinboxPress;
			showMessage();

			if (board->autoCloseRows)
				onOkButtonPressed();
		} else {
			okButton->setVisible(false);
			okButton->setEnabled(false);

			if (playedMoves == 0)
				state = State::WaittingFirstRowFill;
			else
				state = State::WaittingCodeRowFill;

			showMessage();
		}
	} else {	//rules->mode == Rules::Mode::MVH
		doneButton->setVisible(m_filled);

		if (m_filled)//the user is not done putting the master code
			state = State::WaittingDoneButtonPress;
		else
			state = State::WaittingHiddenCodeFill;

		showMessage();
	}
}

void Game::createBoxes()
{
	/*	This function creates boxes and put them in QLists. They are
	 *	stored in QList from bottom to top and left to right.
	 */
	codeBoxes.clear();
	pinBoxes.clear();
	pegBoxes.clear();
	currentBoxes.clear();
	masterBoxes.clear();

	QPoint left_bottom_corner(4, 489);

	for (int i = 0; i < Rules::MAX_COLOR_NUMBER; ++i) {
		QPoint position = left_bottom_corner - QPoint(0, i*40);

		auto pinbox = new PinBox(rules->pegs, position);
		scene()->addItem(pinbox);
		pinBoxes.append(pinbox);

		position.setX(160-20*rules->pegs);

		for (int j = 0; j < rules->pegs; ++j) {
			codeBoxes.append(createPegBox(position+QPoint(j*40, 0)));
		}

		position.setX(277);	//go to right corner for the peg boxes

		PegBox *pegbox = createPegBox(position);
		pegBoxes.append(pegbox);
		if (i < rules->colors) {
			Peg *peg;
			peg = createPeg(pegbox, i);
			peg->setState(Peg::State::Underneath);
			connect(this, SIGNAL(showIndicatorsSignal()), peg, SLOT(onShowIndicators()));
			createPegForBox(pegbox, i);
			pegbox->setPegState(Peg::State::Initial);
		} else {
			createPeg(pegbox, i)->setState(Peg::State::Plain);
		}
	}

	// the last code boxes are for the master code
	for (int i = 0; i < rules->pegs; ++i) {
		masterBoxes.append(createPegBox(QPoint(160-20*rules->pegs+i*40, 70)));
	}
}

void Game::createPegForBox(PegBox *m_box, int m_color)
{
	QPointF pos = m_box->sceneBoundingRect().center();
	if (m_box->hasPeg()) {
		m_box->setPegColor(m_color);
	} else {
		Peg *peg = createPeg(pos, m_color);
		m_box->setPeg(peg);
		connect(peg, SIGNAL(mouseReleaseSignal(Peg *)), this, SLOT(onPegMouseReleased(Peg *)));
		connect(peg, SIGNAL(mouseDoubleClickSignal(Peg*)), this, SLOT(onPegMouseDoubleClicked(Peg*)));
		connect(this, SIGNAL(showIndicatorsSignal()), peg, SLOT(onShowIndicators()));
	}
}

PegBox *Game::createPegBox(const QPoint &m_position)
{
	auto pegbox = new PegBox(m_position);
	scene()->addItem(pegbox);
	return pegbox;
}

Peg *Game::createPeg(const QPointF &m_position, const int &m_color)
{
	auto peg = new Peg(m_position, m_color, &board->indicator);
	scene()->addItem(peg);
	return peg;
}

Peg *Game::createPeg(PegBox *m_box, const int &m_color)
{
	return createPeg(m_box->sceneBoundingRect().center(), m_color);
}

void Game::freezeScene()
{
	setStateOfList(&masterBoxes, Box::State::Past);
	setStateOfList(&currentBoxes, Box::State::Past);
	pinBoxes.at(playedMoves)->setState(Box::State::Past);
	setStateOfList(&pegBoxes, Box::State::Future);
}

void Game::setNextRowInAction()
{
	currentBoxes.clear();
	for(int i = playedMoves*rules->pegs; i < (playedMoves+1)*rules->pegs; ++i)
		currentBoxes.append(codeBoxes.at(i));
	setStateOfList(&currentBoxes, Box::State::Current);
	state = State::WaittingCodeRowFill;
}

void Game::getNextGuess()
{
	state = State::Thinking;
	emit startGuessingSignal();
}

Game::Player Game::winner() const
{
	if (pinBoxes.at(playedMoves)->getValue() == (rules->pegs + 1)*(rules->pegs + 2)/2 - 1)
		return Player::CodeBreaker;
	else if (playedMoves >= Rules::MAX_COLOR_NUMBER - 1)
		return Player::CodeMaker;
	else
		return Player::None;
}

void Game::initializeScene()
{
	codeBoxes.clear();
	pinBoxes.clear();
	pegBoxes.clear();
	masterBoxes.clear();
	currentBoxes.clear();
	playedMoves = 0;

	scene()->clear();
	setInteractive(true);

	okButton = new Button(board->font, 36, tr("OK"));
	scene()->addItem(okButton);
	okButton->setZValue(2);
	connect(okButton, SIGNAL(buttonPressed()), this, SLOT(onOkButtonPressed()));
	okButton->setEnabled(false);
	okButton->setVisible(false);

	doneButton = new Button(board->font, 158, tr("Done"));
	doneButton->setPos(79, 118);
	doneButton->setVisible(false);
	doneButton->setZValue(2);
	doneButton->setEnabled(false);
	connect(doneButton, SIGNAL(buttonPressed()), this, SLOT(onDoneButtonPressed()));
	scene()->addItem(doneButton);

	message = new Message(board->font, "#303030");
	scene()->addItem(message);
	message->setPos(20, 0);

	information = new Message(board->font, "#808080", 4);
	scene()->addItem(information);
	information->setPos(20, 506);
	showInformation();
	createBoxes();
	scene()->update();
}

void Game::onPegMouseReleased(Peg *peg)
{
	QPointF position = peg->sceneBoundingRect().center();
	int color = peg->getColor();
	//if same color is not allowed and there is already a color-peg visible, we just ignore drop
	if(!rules->same_colors) {
		foreach(PegBox *box, currentBoxes) {
			if(!box->sceneBoundingRect().contains(position) &&
					box->isPegVisible() && box->getPegColor() == color) {
				board->sounds.playPegDropRefuse();
				return;
			}
		}
	}

	static bool rowFillState = false;
	bool newRowFillState = true;

	// conversion from float to integer may cause double drop on middle. Flag to do it just once
	bool dropOnlyOnce = true;

	foreach(PegBox *box, currentBoxes) {
		if (box->sceneBoundingRect().contains(position) && dropOnlyOnce) {
			dropOnlyOnce = false;
			createPegForBox(box, color);
			board->sounds.playPegDrop();

			box->setPegState(Peg::State::Visible);
		}//	end if

		if (box->getPegState() != Peg::State::Visible)
			newRowFillState = false;
	} //	end for

	//	if (master) code row state changed, go to codeRowFilled
	if (newRowFillState != rowFillState) {
		rowFillState = newRowFillState;
		codeRowFilled(rowFillState);
	}
}

void Game::onPegMouseDoubleClicked(Peg *peg)
{
	foreach(PegBox *box, currentBoxes) {
		if (!box->hasPeg() || box->getPegState() == Peg::State::Invisible) {
			peg->setPos(box->sceneBoundingRect().topLeft());
			break;
		}
	}
}

void Game::changeIndicators()
{
	emit showIndicatorsSignal();
}

void Game::retranslateTexts()
{
	if(okButton) {
		okButton->setLabel(tr("OK"));
		okButton->update();
	}
	if (doneButton) {
		doneButton->setLabel(tr("Done"));
		doneButton->update();
	}
	showInformation();
	showMessage();
}

bool Game::isRunning()
{
	switch (state) {
	case State::None:
	case State::Win:
	case State::Lose:
	case State::Resign:
	case State::WaittingFirstRowFill:
	case State::WaittingHiddenCodeFill:
	case State::WaittingDoneButtonPress:
		return false;
	default:
		return true;
	}
}

void Game::onRevealOnePeg()
{
	if (rules->mode == Rules::Mode::HVM && isRunning()) {
		foreach(PegBox *box, masterBoxes) {
			if(box->getState() != Box::State::Past) {
				if (box == masterBoxes.last()) {
					onResigned();
				} else {
					box->setState(Box::State::Past);
					return;
				}
			}
		}
	}
}

void Game::onResigned()
{
	if (rules->mode == Rules::Mode::HVM && isRunning()) {
		state = State::Resign;
		showMessage();
		freezeScene();
	}
}

void Game::onOkButtonPressed()
{
	if(rules->mode == Rules::Mode::MVH) {
		int resp = pinBoxes.at(playedMoves)->getValue();
		if(!solver->setResponse(resp)) {
			message->setText(tr("Not Possible, Try Again"));
			return;
		}
		board->sounds.playButtonPress();
		okButton->setVisible(false);

		pinBoxes.at(playedMoves)->setState(Box::State::Past);

		switch (winner()) {
		case Player::CodeBreaker:
			state = State::Win;
			freezeScene();
			break;
		case Player::CodeMaker:
			state = State::Lose;
			freezeScene();
			break;
		default:
			++playedMoves;
			getNextGuess();
			break;
		}

		showMessage();
	} else {
		board->sounds.playButtonPress();
		state = State::Running;

		guess.guess = "";
		for(int i = 0; i < rules->pegs; ++i)
			guess.guess.append(QString::number(currentBoxes.at(i)->getPegColor()));

		pinBoxes.at(playedMoves)->setPins(guess.code, guess.guess, rules->colors);
		pinBoxes.at(playedMoves)->setState(Box::State::Past);

		setStateOfList(&currentBoxes, Box::State::Past);
		currentBoxes.clear();

		switch (winner()) {
		case Player::CodeBreaker:
			state = State::Win;
			freezeScene();
			break;
		case Player::CodeMaker:
			state = State::Lose;
			freezeScene();
			break;
		default:
			++playedMoves;
			setNextRowInAction();
			break;
		}

		showMessage();
		okButton->setVisible(false);
		okButton->setPos(pinBoxes.at(playedMoves)->pos() + QPoint(0, 1));
	}
}

void Game::onDoneButtonPressed()
{
	board->sounds.playButtonPress();
	state = State::Running;

	doneButton->setVisible(false);
	doneButton->setEnabled(false);

	setStateOfList(&currentBoxes, Box::State::Past);
	setStateOfList(&pegBoxes, Box::State::Future);

	guess.code = "";
	foreach(PegBox *box, currentBoxes)
		guess.code.append(QString::number(box->getPegColor()));

	currentBoxes.clear();
	getNextGuess();
	showMessage();
}

void Game::onGuessReady()
{
	state = State::Running;
	showInformation();

	int box_index = playedMoves*rules->pegs;
	for(int i = 0; i < rules->pegs; ++i) {
		createPegForBox(codeBoxes.at(box_index + i), guess.guess[i].digitValue());
		codeBoxes.at(box_index + i)->setState(Box::State::Past);
	}
	state = State::WaittingOkButtonPress;
	showMessage();
	pinBoxes.at(playedMoves)->setState(Box::State::None);
	okButton->setEnabled(true);
	okButton->setVisible(true);
	okButton->setPos(pinBoxes.at(playedMoves)->pos()-QPoint(0, 39));

	if (board->autoPutPins)
		pinBoxes.at(playedMoves)->setPins(guess.code, guess.guess, rules->colors);
}

void Game::play()
{
	stop();

	if(rules->mode == Rules::Mode::MVH)
		playMVH();
	else // (rules->mode == Rules::Mode::HVM)
		playHVM();
}

void Game::stop()
{
	if (solver) {
		emit interuptSignal();
		solver->quit();
		solver->wait();
	}
	initializeScene();
	state = State::None;
}

void Game::playMVH()
{
	doneButton->setZValue(2);
	doneButton->setVisible(false);
	doneButton->setEnabled(true);

	if (!solver) {
		solver = new Solver(rules, &guess, this);
		connect(solver, SIGNAL(guessDoneSignal()), this, SLOT(onGuessReady()));
		connect(this, SIGNAL(startGuessingSignal()), solver, SLOT(onStartGuessing()));
		connect(this, SIGNAL(resetGameSignal()), solver, SLOT(onReset()));
		connect(this, SIGNAL(interuptSignal()), solver, SLOT(onInterupt()));
	}
	emit interuptSignal();
	emit resetGameSignal();

	state = State::WaittingHiddenCodeFill;
	showMessage();
	showInformation();

	//initializing currentrow
	for(int i = 0; i < rules->pegs; ++i) {
		currentBoxes.append(masterBoxes.at(i));
		masterBoxes.at(i)->setState(Box::State::Current);
	}

	/*	Nothing happening from here till the user fill the master code
	 *	row and press the done button. After the done button is pressed,
	 *	the onDoneButtonPressed is continuing the game
	 */
}

void Game::playHVM()
{
	QString digits = "0123456789";
	digits.left(rules->colors);
	int remainingNumbers = rules->colors;
	qsrand(time(NULL));
	guess.code = "";

	//creating a master code to be guessed
	foreach(PegBox *box, masterBoxes) {
		int color = static_cast<int>(remainingNumbers*(qrand()/(RAND_MAX + 1.0)));
		int realcolor = digits.at(color).digitValue();
		guess.code.append(QString::number(realcolor));
		createPegForBox(box, realcolor);
		box->setState(Box::State::None);
		if(!rules->same_colors) {
			digits.remove(color, 1);
			--remainingNumbers;
		}
	}
	setNextRowInAction();
	showMessage();
	showInformation();
	state = State::WaittingFirstRowFill;
	okButton->setPos(pinBoxes.at(playedMoves)->pos() + QPoint(0, 1));
	// from now on the onPinBoxPushed function continue the game, after the code row is filled
}

void Game::resizeEvent(QResizeEvent *event)
{
	fitInView(sceneRect(), Qt::KeepAspectRatio);
	QGraphicsView::resizeEvent(event);
}

void Game::setAlgorithm(const Rules::Algorithm &algorithm_n)
{
	rules->algorithm = algorithm_n;
}

void Game::showInformation()
{
	if (rules->mode == Rules::Mode::MVH) {
		if (guess.possibles == 1)
		{
			information->setText(tr("The Code Is Cracked!"));
		} else if (guess.possibles > 10000) {
			information->setText(QString("%1	%2: %3").arg(tr("Random Guess")).
								  arg(tr("Remaining")).arg(board->locale.toString(guess.possibles)));
		} else {
			switch (guess.algorithm) {
			case Rules::Algorithm::MostParts:
				information->setText(QString("%1: %2	%3: %4").arg(tr("Most Parts")).
										  arg(board->locale.toString(guess.weight)).arg(tr("Remaining")).
										  arg(board->locale.toString(guess.possibles)));
				break;
			case Rules::Algorithm::WorstCase:
				information->setText(QString("%1: %2	%3: %4").arg(tr("Worst Case")).
										  arg(board->locale.toString(guess.weight)).arg(tr("Remaining")).
										  arg(board->locale.toString(guess.possibles)));
				break;
			default:
				information->setText(QString("%1: %2	%3: %4").arg(tr("Expected Size")).
										  arg(board->locale.toString(guess.weight)).arg(tr("Remaining")).
										  arg(board->locale.toString(guess.possibles)));
				break;
			}

		}
	} else {
		information->setText(QString("%1: %2   %3: %4   %5: %6").arg(tr("Slots", "", rules->pegs)).
								  arg(board->locale.toString(rules->pegs)).arg(tr("Colors", "", rules->colors)).
								  arg(board->locale.toString(rules->colors)).arg(tr("Same Colors")).
							 arg(rules->same_colors ? tr("Yes"): tr("No")));
	}
}

void Game::showMessage()
{
	bool is_MVH = (rules->mode == Rules::Mode::MVH);
	switch (state) {
	case State::Win:
		if (is_MVH)
			message->setText(tr("Success! I Win"));
		else
			message->setText(tr("Success! You Win"));
		break;
	case State::Lose:
		if (is_MVH)
			message->setText(tr("Game Over! I Failed"));
		else
			message->setText(tr("Game Over! You Failed"));
		break;
	case State::Resign:
		message->setText(tr("You Resign"));
		break;
	case State::Thinking:
		message->setText(tr("Let Me Think"));
		break;
	case State::WaittingPinboxPress:
		message->setText(tr("Press OK"));
		break;
	case State::WaittingOkButtonPress:
		message->setText(tr("Please Put Your Pins And Press OK"));
		break;
	case State::WaittingDoneButtonPress:
		message->setText(tr("Press Done"));
		break;
	default:
		message->setText(tr("Place Your Pegs"));
		break;
	}
}
void Game::drawBackground(QPainter *painter, const QRectF &rect)
{
	painter->fillRect(rect, QColor(200, 200, 200));// set scene background color
	painter->setPen(Qt::NoPen);

	QRectF cr(3, 3, 314, 554);
	QPainterPath cpath;
	cpath.addRoundedRect(cr, 10.1, 10.1);
	painter->setClipPath(cpath);
	painter->setClipping(true);

	QLinearGradient top_grad(0, 16, 0, 129);
	top_grad.setColorAt(0.0, QColor(248, 248, 248));
	top_grad.setColorAt(0.6, QColor(184, 184, 184));
	top_grad.setColorAt(1, QColor(212, 212, 212));
	painter->setBrush(QBrush(top_grad));
	painter->drawRect(QRect(4, 4, 312, 125));
	painter->setBrush(QBrush(QColor(112, 112, 112)));
	painter->drawRect(QRect(4, 128, 318, 1));

	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(QColor(150, 150, 150)));
	painter->drawRect(QRectF(4, 129, 318, 400));

	QLinearGradient bot_grad(0, 530, 0, 557);
	bot_grad.setColorAt(0.0, QColor(204, 204, 204));
	bot_grad.setColorAt(0.3, QColor(206, 206, 206));
	bot_grad.setColorAt(1.0, QColor(180, 180, 180));
	painter->setBrush(QBrush(bot_grad));
	painter->drawRect(QRect(1, 529, 318, 28));
	painter->setBrush(QBrush(QColor(239, 239, 239)));
	painter->setClipping(false);

	QLinearGradient frame_grad(0, 190, 320, 370);
	frame_grad.setColorAt(0.0, QColor(240, 240, 240));
	frame_grad.setColorAt(0.49, QColor(240, 240, 240));
	frame_grad.setColorAt(0.50, QColor(80, 80, 80));
	frame_grad.setColorAt(1.0, QColor(80, 80, 80));
	QPen frame_pen = QPen(QBrush(frame_grad), 1.5);
	QRectF right_shadow(3.5, 3.5, 313, 553);
	painter->setBrush(Qt::NoBrush);
	painter->setPen(frame_pen);
	painter->drawRoundedRect(right_shadow, 9.8, 9.8);

	QLinearGradient sol_frame_grad = QLinearGradient(50, 70, 50, 110);
	sol_frame_grad.setColorAt(0.0, QColor(80, 80, 80));
	sol_frame_grad.setColorAt(1.0, QColor(255, 255, 255));
	QPen sol_frame_pen(QBrush(sol_frame_grad), 1);

	painter->setPen(sol_frame_pen);

	QRectF sol_container(41, 68, 235, 42);
	QRectF sol_frame(42, 69, 235, 41.5);

	painter->drawRoundedRect(sol_container, 21,21);
	painter->setBrush(QColor(150, 150, 150));
	painter->drawRoundedRect(sol_frame, 20, 20);
	painter->setRenderHint(QPainter::TextAntialiasing, true);
}
