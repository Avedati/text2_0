// TODO: make file i/o more flexible
// TODO: implement selection
// TODO: implement syntax highlighting
// TODO: support resizing
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#define CTRL(x) (x & 0x1f)
#define MIN(x, y) (x < y) ? x : y
#define MAX(x, y) (x < y) ? y : x

#define LINE_NUMBER_SPACES 4
#define TAB_SPACES 4

char* buffer;
long n_buffer;

int cursor_x;
int cursor_y;

int view_x;
int view_y;

int text_cols;
int text_rows;

int select_start_x;
int select_start_y;

char* filename;
FILE* fp;
int dirty;

void print_line_nos(void) {
	for (int i = 0; i < text_rows; i++) {
		mvprintw(i, 0, "%d", i + 1 + view_y);
	}
}

void print_buffer(void) {
	int x = 0;
	int y = 0;
	for (int i = 0; i < n_buffer; i++) {
		if (buffer[i] == '\n') {
			x = 0;
			y++;
		}
		else {
			int ch_x = x + LINE_NUMBER_SPACES - view_x;
			int ch_y = y - view_y;
			if (ch_x >= LINE_NUMBER_SPACES && ch_x < text_cols &&
			    ch_y >= 0 && ch_y < text_rows) {
				mvaddch(ch_y, ch_x, buffer[i]);
			}
			x++;
		}
	}
}

void print_status(void) {
	mvprintw(text_rows + 1, 0, "file: %s%s", filename == NULL ? "[no file]" : filename, dirty ? " [dirty]" : "");
}

int xy_to_index(int cx, int cy) {
	int x = 0;
	int y = 0;
	int i;
	for (i = 0; i < n_buffer; i++) {
		if (x == cx && y == cy) { return i; }
		if (buffer[i] == '\n') {
			x = 0;
			y++;
		}
		else {
			x++;
		}
	}
	if (x == cx && y == cy) { return i; }
	return -1;
}

void index_to_xy(int index, int* cx, int* cy) {
	int x = 0;
	int y = 0;
	int i;
	for (i = 0; i < n_buffer; i++) {
		if (i == index) { break; }
		if (buffer[i] == '\n') {
			x = 0;
			y++;
		}
		else {
			x++;
		}
	}
	if (i == index) {
		*cx = x;
		*cy = y;
	}
}

int line_width(int cy) {
	int y = 0;
	int i;
	for (i = 0; i < n_buffer; i++) {
		if (y == cy) { break; }
		if (buffer[i] == '\n') {
			y++;
		}
	}
	if (i == n_buffer) { return -1; }
	int result = 0;
	for (; i < n_buffer; i++) {
		if (buffer[i] == '\n') { break; }
		result++;
	}
	return result;
}

int num_lines(void) {
	int result = 1;
	for (int i = 0; i < n_buffer; i++) {
		if (buffer[i] == '\n') { result++; }
	}
	return result;
}

int at_view_end(int vx, int vy) {
	int rel_cursor_x = cursor_x - view_x;
	int rel_cursor_y = cursor_y - view_y;
	if (rel_cursor_x == 0 && vx < 0) {
		return 1;
	}
	if (rel_cursor_x == text_cols - LINE_NUMBER_SPACES - 1 && vx > 0) {
		return 1;
	}
	if (rel_cursor_y == 0 && vy < 0) {
		return 1;
	}
	if (rel_cursor_y == text_rows - 1 && vy > 0) {
		return 1;
	}
	return 0;
}

void insert_char(char c) {
	buffer = realloc(buffer, sizeof(char) * (n_buffer + 2));
	int index = xy_to_index(cursor_x, cursor_y);
	for (int i = n_buffer - 1; i >= index; i--) {
		buffer[i + 1] = buffer[i];
	}
	buffer[index] = c;
	buffer[++n_buffer] = 0;
}

int update(void) {
	clear();
	print_line_nos();
	print_buffer();
	print_status();
	move(cursor_y - view_y, cursor_x + LINE_NUMBER_SPACES - view_x);
	refresh();

	int c = getch();
	if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')) {
		insert_char(c);
		if (at_view_end(1, 0)) {
			view_x++;
		}
		cursor_x++;
		dirty = 1;
	}
	else if (strchr("!@#$%^&*()_+-={}[]|\\:\";',./<>?~` ", c) != NULL) {
		insert_char(c);
		if (at_view_end(1, 0)) {
			view_x++;
		}
		cursor_x++;
		dirty = 1;
	}
	else if (c == '\t') {
		buffer = realloc(buffer, sizeof(char) * (n_buffer + TAB_SPACES + 1));
		for (int i = 0; i < TAB_SPACES; i++) {
			insert_char(' ');
		}
		if (at_view_end(1, 0)) {
			view_x += TAB_SPACES;
		}
		cursor_x += TAB_SPACES;
		dirty = 1;
	}
	else if (c == '\n') {
		insert_char(c);
		if (at_view_end(0, 1)) {
			view_y++;
		}
		cursor_x = 0;
		cursor_y++;
		dirty = 1;
	}
	else if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
		int cursor_pos = xy_to_index(cursor_x, cursor_y);
		if (cursor_pos != -1 && 0 < cursor_pos && cursor_pos <= n_buffer) {
			for (int i = cursor_pos - 1; i < n_buffer; i++) {
				buffer[i] = buffer[i + 1];
			}
			buffer[--n_buffer] = 0;
			if (at_view_end(0, -1) && cursor_y != 0) {
				view_y--;
			}
			if (at_view_end(-1, 0) && cursor_x != 0) {
				view_x--;
			}
			index_to_xy(cursor_pos - 1, &cursor_x, &cursor_y);
		}
		dirty = 1;
	}
	else if (c == KEY_LEFT) {
		int cursor_pos = xy_to_index(cursor_x, cursor_y);
		if (at_view_end(-1, 0) && cursor_x != 0) {
			view_x--;
		}
		index_to_xy(MAX(0, cursor_pos - 1), &cursor_x, &cursor_y);
	}
	else if (c == KEY_RIGHT) {
		int cursor_pos = xy_to_index(cursor_x, cursor_y);
		if (at_view_end(1, 0) && cursor_x != line_width(cursor_y)) {
			view_x--;
		}
		index_to_xy(MIN(n_buffer, cursor_pos + 1), &cursor_x, &cursor_y);
	}
	else if (c == KEY_UP) {
		if (cursor_y > 0) {
			if (at_view_end(0, -1)) {
				view_x = MAX(0, line_width(cursor_y - 1) - (text_cols - LINE_NUMBER_SPACES - 1));
				view_y--;
			}
			cursor_x = MIN(line_width(cursor_y - 1), cursor_x);
			cursor_y--;
		}
	}
	else if (c == KEY_DOWN) {
		if (cursor_y < num_lines() - 1) {
			if (at_view_end(0, 1)) {
				view_x = MAX(0, line_width(cursor_y + 1) - (text_cols - LINE_NUMBER_SPACES - 1));
				view_y++;
			}
			cursor_x = MIN(line_width(cursor_y + 1), cursor_x);
			cursor_y++;
		}
	}
	else if (c == CTRL('q') && !dirty) {
		return 0;
	}
	else if (c == CTRL('s')) {
		fseek(fp, 0, SEEK_SET);
		fwrite(buffer, n_buffer, sizeof(char), fp);
		dirty = 0;
	}
	return 1;
}

int main(int argc, char** argv) {
	if (argc >= 2) {
		filename = malloc(sizeof(char) * (strlen(argv[1]) + 1));
		strcpy(filename, argv[1]);

		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "error: could not open file %s\n", argv[1]);
			return 1;
		}
		fseek(fp, 0, SEEK_END);
		n_buffer = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buffer = malloc(sizeof(char) * (n_buffer + 1));
		fread(buffer, n_buffer, 1, fp);
		buffer[n_buffer] = 0;
		fclose(fp);
		fp = fopen(argv[1], "w");
		if (fp == NULL) {
			fprintf(stderr, "error: could not open file %s\n", argv[1]);
			return 1;
		}

		cursor_x = 0;
		cursor_y = 0;
		view_x = 0;
		view_y = 0;
		select_start_x = -1;
		select_start_y = -1;
		dirty = 0;

		initscr();
		text_cols = COLS;
		text_rows = LINES - 2;
		keypad(stdscr, TRUE);
		raw();
		noecho();
		while (update());
		endwin();

		fseek(fp, 0, SEEK_SET);
		fwrite(buffer, n_buffer, sizeof(char), fp);
		fclose(fp);
		free(buffer);
		free(filename);	
	}
	else {
		fprintf(stderr, "error: no filename provided.\n");
	}
	return 0;
}
