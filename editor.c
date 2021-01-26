#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define CTRL_KEY(k) ((k) & 0x1f)

typedef struct {
	char *array;
	size_t used;
	size_t size;
} Array;

void initArray(Array *a, size_t inititalSize) {
	a->array = malloc(inititalSize * sizeof(int));
	a->used = 0;
	a->size = inititalSize;
}
void insertArray(Array *a, int element, int pos) {
	if (a->used == a->size) {
		a->size *= 2;
		a->array = realloc(a->array, a->size*sizeof(int));
	}
	for (int i = a->used-1; i >= pos; i--) {
		a->array[i+1] = a->array[i];
	}
	a->array[pos] = element;
	a->used++;
	// a->array[a->used++] = element;
}
void removeArray(Array *a, int pos) {
	if (a->used != 0) {
		for (int i = pos+1; i <= a->used;i++) {
			a->array[i-1] = a->array[i];
		}
		a->used--;
	}
}
void freeArray(Array *a) {
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}

void writeToFile(const char* fileName, const char* data, const int itemCount) {
	FILE *fp = fopen(fileName, "w");
	fseek(fp, 0, SEEK_SET);
	fwrite(data, sizeof(char), itemCount, fp);
	fclose(fp);
}
int main(int argc, char const *argv[])
{
	if (argc != 2) {
		printf("Usage: edit <file name>");
		return 0;
	}
	FILE *file = fopen(argv[1], "r+");
	if (file == NULL) {
		file = fopen(argv[1], "w");
		if (file == NULL) {
			printf("Error creating file\n");
			return 1;
		}
	}
	

	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	cbreak();
	curs_set(0);
	size_t WIDTH;
	size_t HEIGHT;
	getmaxyx(stdscr, HEIGHT, WIDTH);

	
	fseek(file, 0, SEEK_END);
	size_t length = ftell(file);
	rewind(file);

	Array data;
	initArray(&data, length+1);
	Array lineLengths;
	initArray(&lineLengths, length+1);

	int lineCount = 0;

	int currentChar;
	while ((currentChar = getc(file)) != EOF) {
		lineLengths.array[lineCount]++;
		if (currentChar == '\n') {
			lineCount++;
		}
		insertArray(&data, currentChar, data.used);
	}
	fclose(file);
	lineLengths.used = lineCount+1;
	int cursorCol = 0;
	int cursorLine = 0;
	int cursorPos = 0;
	int scroll = 0;
	int input = -1;
	erase();
	do {
		erase();
		int i = 0;
		if (scroll != 0) { // calculate how much data to skip based on how many lines scrolled down
			for (int j = 0; j < scroll; j++) {
				i += lineLengths.array[j];
			}
		}
		for (int line = scroll; line < lineLengths.used && line < scroll+HEIGHT - 1; line++) {
			int lineLen = lineLengths.array[line];
			printw("%3d ", line+1);
			if (line == cursorLine) {
				addch('+');
			}
			else {
				addch('|');
			}
			addch(' ');
			// j = col line = line i = position in whole data
			for (int j = 0; j <= lineLen; j++) {
				
				if (j == cursorCol && line == cursorLine) {
					cursorPos = i;
					addch(ACS_VLINE);
				}
				if (i < data.used) {
					if (data.array[i] == '\n') {
						addch('\n');
						i++;
						break;
					}
					addch(data.array[i++]);
				}
			}
		}
		if (lineLengths.used - scroll < HEIGHT) {
			for (int i = 0; i < HEIGHT ; i++) {
				printw("\n%5c", '~');
			}
		}
		printw("%s\t%d,%d", argv[1] ,cursorCol, cursorLine);
		refresh();
		input = getch();
		switch (input) {
			case KEY_UP: {
				if (cursorLine > 0) {
					cursorLine--;
					if (cursorCol > lineLengths.array[cursorLine]) {
						cursorCol = lineLengths.array[cursorLine]-1;
					}
				}
				if (cursorLine == scroll && scroll > 0) {
					scroll--;
				}
				break;
			}
			case KEY_DOWN: {
				if (cursorLine < lineLengths.used-1) {
					cursorLine++;
					if (cursorCol > lineLengths.array[cursorLine]) {
						cursorCol = lineLengths.array[cursorLine]-1;
					}
				}
				if (cursorLine - scroll == HEIGHT) {
					scroll++;
				}
				break;
			}
			case KEY_LEFT: {
				if (cursorCol == 0) {
					if (cursorLine > 0) {
						cursorLine--;
						cursorCol = lineLengths.array[cursorLine]-1;
					}
				} else {
					cursorCol--;
				}
				break;
			}
			case KEY_RIGHT: {
				if (cursorCol == lineLengths.array[cursorLine] && cursorLine < lineLengths.used-1) {
					cursorLine++;
					cursorCol = 0;
				} else {
					if (cursorCol < lineLengths.array[cursorLine]) {
						if (data.array[cursorPos] == '\n') {
							cursorLine++;
							cursorCol = 0;
						}
						else {
							cursorCol++;							
						}
					}
				}
				break;
			}
			case KEY_DC: {
				if (cursorCol < lineLengths.array[cursorLine]-1) {
					lineLengths.array[cursorLine]--;
					removeArray(&data, cursorPos);
				}
				break;
			}
			case KEY_BACKSPACE:
			case 127: {
				if (cursorCol == 0 && cursorLine == 0) {
					break;
				}
				if (data.array[cursorPos-1] == '\n') {
					// //add the lines length onto the previous lines length then remove the length form the length array
					lineLengths.array[cursorLine-1] += lineLengths.array[cursorLine] - 1; // -1 because we deleted a character
					removeArray(&lineLengths, cursorLine);
					cursorLine--;
					cursorCol = lineLengths.array[cursorLine]-1;
					lineCount--;
				}
				else {
					lineLengths.array[cursorLine]--;
					cursorCol--;
				}
				removeArray(&data, cursorPos-1);
				if (cursorLine == scroll && scroll > 0) {
					scroll--;
				}
				break;
			}
			case '\n': {
				int count = lineLengths.array[cursorLine] - cursorCol;
				insertArray(&data, '\n', cursorPos);
				insertArray(&lineLengths, count, cursorLine+1);
				cursorLine++;
				cursorCol = 0;
				if (cursorLine - scroll == HEIGHT) {
					scroll++;
				}
				break;
			}
			case CTRL_KEY('s'): {
				writeToFile(argv[1], data.array, data.used);
				break;
			}
			default: {
				insertArray(&data, input, cursorPos);
				lineLengths.array[cursorLine]++;
				cursorCol++;
				break;
			}
		}
	} while (input != CTRL_KEY('q'));
	erase();
	endwin();
	//writeToFile(fileName, data, strlen(data));
	freeArray(&data);
	freeArray(&lineLengths);
	return 0;
}
