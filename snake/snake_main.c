#include <time.h>
//#include <ncurses.h>
#include "graphics/curses.h"
#include <unistd.h>
#include <stdlib.h>

#include "debug.h"
#include <fcntl.h>

#define DELAY 30000
#define TIMEOUT 10 

/* Global Vars */
	typedef enum {
		LEFT,
		RIGHT,
		UP,
		DOWN
	} direction_type;

	typedef struct point {
		int x;
		int y;
	} point;

	int x = 0,
		y = 0,
		maxY = 0, 
		maxX = 0,
		nextX = 0,
		nextY = 0,
		tailLength = 5,
		score = 0;

	bool gameOver = false;

	direction_type currentDir = RIGHT;
	point snakeParts[255] = {};
	point food;

/* Functions */
	void createFood() {
		//Food.x is a random int between 10 and maxX - 10
		food.x = (rand() % (maxX - 20)) + 10;

		//Food.y is a random int between 5 and maxY - 5
		food.y = (rand() % (maxY - 10)) + 5;
	}
	
	void drawPart(point drawPoint) {
		mvprintw(drawPoint.y, drawPoint.x, "o");
	}

	void cursesInit() {
		initscr(); //Initialize the window
		noecho(); //Don't echo keypresses
		keypad(stdscr, TRUE);
		cbreak();
		timeout(TIMEOUT);
		curs_set(FALSE); //Don't display a cursor

		//Global var stdscr is created by the call to initscr()
		getmaxyx(stdscr, maxY, maxX);
	}

	void init() {
		srand(time(NULL));

		currentDir = RIGHT;
		tailLength = 5;
		gameOver = false;
		score = 0;

		clear(); //Clears the screen
		
		//Set the initial snake coords 
		int j = 0;
		for(int i = tailLength; i >= 0; i--) {
			point currPoint;
			
			currPoint.x = i;
			currPoint.y = maxY / 2; //Start mid screen on the y axis

			snakeParts[j] = currPoint;
			j++;
		}


		createFood();

		refresh();
	}

	void shiftSnake() {
		point tmp = snakeParts[tailLength - 1];

		for(int i = tailLength - 1; i > 0; i--) {
			snakeParts[i] = snakeParts[i-1];
		}

		snakeParts[0] = tmp;
	}

	void drawScreen() {
		//Clears the screen - put all draw functions after this
		clear(); 

		//Print game over if gameOver is true
		if(gameOver)
			mvprintw(maxY / 2, maxX / 2, "Game Over!");

		//Draw the snake to the screen
		for(int i = 0; i < tailLength; i++) {
			drawPart(snakeParts[i]);
		}

		//Draw the current food
		drawPart(food);

		//Draw the score
		mvprintw(1, 2, "Score: %i", score);

		//ncurses refresh
		refresh();

		//Delay between movements
		usleep(DELAY); 
	}

#define SPRESENSE_MAIN_BOARD_UART_DEVICE_PATH "/dev/ttyS0"

int getch_serial(int fd) {
	int len;
	int ch = 0;

	len = read(fd, (void*)&ch, 1);
    if(len > 0)
    {
		if (ch == 0x1b) {
			read(fd, &ch, 1);
			if (ch == 0x5b) {
				read(fd, &ch, 1);
				if (ch == 0x44) ch = KEY_LEFT;
				if (ch == 0x43) ch = KEY_RIGHT;
				if (ch == 0x41) ch = KEY_UP;
				if (ch == 0x42) ch = KEY_DOWN;
			}
		}
    }

	return ch;
}

/* Main */
	int main(int argc, char *argv[]) {
		_info("snake main()\n");

		int fd = open(SPRESENSE_MAIN_BOARD_UART_DEVICE_PATH, O_RDWR | O_NONBLOCK);
		if (fd < 0)
		{
			printf("%s open error.\n", SPRESENSE_MAIN_BOARD_UART_DEVICE_PATH);
			return 1;
		}

		cursesInit();
		init();

		int ch;
		while(1) {
			//Global var stdscr is created by the call to initscr()
			//This tells us the max size of the terminal window at any given moment
			getmaxyx(stdscr, maxY, maxX);
			
			if(gameOver) {
				sleep(2);
				init();
			}

			/* Input Handler */
				//ch = getch();
				ch = getch_serial(fd);

				if(( ch=='l' || ch=='L' || ch == KEY_RIGHT) && (currentDir != RIGHT && currentDir != LEFT)) {
					currentDir = RIGHT;
				} else if (( ch=='h' || ch=='H' || ch == KEY_LEFT) && (currentDir != RIGHT && currentDir != LEFT)) {
					currentDir = LEFT;
				} else if((ch=='j' || ch=='J' || ch == KEY_DOWN) && (currentDir != UP && currentDir != DOWN)) {
					currentDir = DOWN;
				} else if((ch=='k' || ch=='K' || ch == KEY_UP) && (currentDir != UP && currentDir != DOWN)) {
					currentDir = UP;
				} else if ((ch=='q' || ch=='Q')) {
					break;
				}

				_info("ch = getch() char:%c, hex:%x, dec:%d\n", ch, ch, ch);

			/* Movement */
				nextX = snakeParts[0].x;
				nextY = snakeParts[0].y;

				if(currentDir == RIGHT) nextX++;
		    	else if(currentDir == LEFT) nextX--;
				else if(currentDir == UP) nextY--;
				else if(currentDir == DOWN) nextY++;

				if(nextX == food.x && nextY == food.y) {
					point tail;
					tail.x = nextX;
					tail.y = nextY;

					snakeParts[tailLength] = tail;

					if(tailLength < 255)
						tailLength++;
					else
						tailLength = 5; //If we have exhausted the array then just reset the tail length but let the player keep building their score :)

					score += 5;
					createFood();
				} else {
					//Draw the snake to the screen
					for(int i = 0; i < tailLength; i++) {
						if(nextX == snakeParts[i].x && nextY == snakeParts[i].y) {
							gameOver = true;
							break;
						}
					}

					//We are going to set the tail as the new head
					snakeParts[tailLength - 1].x = nextX;
					snakeParts[tailLength - 1].y = nextY;
				}

				//Shift all the snake parts
				shiftSnake();

				//Game Over if the player hits the screen edges
				if((nextX >= maxX || nextX < 0) || (nextY >= maxY || nextY < 0)) {
					gameOver = true;
				}

			/* Draw the screen */
				drawScreen();
		}

		endwin(); //Restore normal terminal behavior
		nocbreak();

		return 0;
	}
