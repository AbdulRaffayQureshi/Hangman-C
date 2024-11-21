#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>
#include <unistd.h> // Include for sleep function
#include <conio.h> // Include for kbhit and getch functions
#include <io.h> // Include for _access function

#define MAX_TRIES 10
#define MAX_WORDS 100
#define MAX_WORD_LENGTH 20

#pragma comment(lib, "winmm.lib")

typedef struct {
    char question[256];
    char options[4][256];
    char correct_option;
} Question;

typedef struct {
    char word[MAX_WORD_LENGTH];
    Question questions[MAX_TRIES];
} WordEntry;

int timed_out = 0;

// Function prototypes
void moveToTopRight();
void clearLine();
void print_hangman(int tries);
void playSound(const char* soundFile);
void ask_question(Question q, int *correct);
void load_words(WordEntry words[], int *num_words);
void choose_word(WordEntry words[], int num_words, char *word, Question questions[]);
void shuffle_questions(Question questions[], int n);
void provide_hint(Question q);
void play_game(char *word, Question questions[], int difficulty);
void print_welcome_screen();
void print_game_over_screen(const char* word);
void print_winning_screen(const char* word, int score);
void pauseGame();

int main() {
    print_welcome_screen();
    WordEntry words[MAX_WORDS];
    int num_words;
    load_words(words, &num_words);

    int difficulty;
    printf("Choose difficulty level (1: Easy, 2: Medium, 3: Hard): ");
    if (scanf("%d", &difficulty) != 1) {
        printf("Invalid input. Exiting.\n");
        return 1;
    }

    // Validate difficulty level
    while (difficulty < 1 || difficulty > 3) {
        printf("Invalid difficulty level. Please choose 1 (Easy), 2 (Medium), or 3 (Hard): ");
        if (scanf("%d", &difficulty) != 1) {
            printf("Invalid input. Exiting.\n");
            return 1;
        }
    }

    // Seed the random number generator once
    srand(time(NULL));

    char word[MAX_WORD_LENGTH];
    Question questions[MAX_TRIES];
    choose_word(words, num_words, word, questions);

    play_game(word, questions, difficulty);

    return 0;
}

// Function definitions
void moveToTopRight() {
    printf("\033[2;70H"); // Move cursor to second row, right corner (assuming 70 columns width)
}

void clearLine() {
    printf("\033[K"); // Clear the line from the cursor to the end
}

void print_hangman(int tries) {
    switch (tries) {
        case 0: printf("  +---+\n      |\n      |\n      |\n     ===\n"); break;
        case 1: printf("  +---+\n  O   |\n      |\n      |\n     ===\n"); break;
        case 2: printf("  +---+\n  O   |\n  |   |\n      |\n     ===\n"); break;
        case 3: printf("  +---+\n  O   |\n /|   |\n      |\n     ===\n"); break;
        case 4: printf("  +---+\n  O   |\n /|\\  |\n      |\n     ===\n"); break;
        case 5: printf("  +---+\n  O   |\n /|\\  |\n /    |\n     ===\n"); break;
        case 6: printf("  +---+\n  O   |\n /|\\  |\n / \\  |\n     ===\n"); break;
        case 7: printf("  +---+\n  O   |\n /|\\  |\n / \\  |\n  |  ===\n"); break;
        case 8: printf("  +---+\n  O   |\n /|\\  |\n / \\  |\n /|  ===\n"); break;
        case 9: printf("  +---+\n  O   |\n /|\\  |\n / \\  |\n / \\ ===\n"); break;
        case 10: printf("  +---+\n [O]  |\n /|\\  |\n / \\  |\n / \\ ===\n"); break;
    }
}

void playSound(const char* soundFile) {
    if (_access(soundFile, 0) == -1) {
        printf("Sound file does not exist: %s\n", soundFile);
        return;
    }
    if (PlaySound(TEXT(soundFile), NULL, SND_FILENAME | SND_SYNC)) {
        printf("Sound played successfully: %s\n", soundFile);
    } else {
        printf("Failed to play sound: %s (Error code: %lu)\n", soundFile, GetLastError());
    }
}

void pauseGame() {
    HANDLE hStdout;
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    COORD coordScreen = { 0, 0 };
    DWORD dwConSize;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
        return;
    }

    dwConSize = csbiInfo.dwSize.X * csbiInfo.dwSize.Y;

    // Save the current screen buffer
    CHAR_INFO *buffer = (CHAR_INFO *)malloc(dwConSize * sizeof(CHAR_INFO));
    SMALL_RECT readRegion = { 0, 0, csbiInfo.dwSize.X - 1, csbiInfo.dwSize.Y - 1 };
    ReadConsoleOutput(hStdout, buffer, csbiInfo.dwSize, coordScreen, &readRegion);

    // Save the current screen buffer size and cursor position
    COORD bufferSize = csbiInfo.dwSize;
    COORD cursorPosition = csbiInfo.dwCursorPosition;

    system("cls"); // Clear the screen
    printf("\n\n\n\n\n\n\n\n\n\n"); // Move the cursor down to the middle of the screen
    printf("\t\t\t\t  _____                        \n");
    printf("\t\t\t\t |  __ \\                       \n");
    printf("\t\t\t\t | |__) |__ _ _ __   __ _ ___  \n");
    printf("\t\t\t\t |  ___/ _` | '_ \\ / _` / __| \n");
    printf("\t\t\t\t | |  | (_| | | | | (_| \\__ \\ \n");
    printf("\t\t\t\t |_|   \\__,_|_| |_|\\__, |___/ \n");
    printf("\t\t\t\t                    __/ |     \n");
    printf("\t\t\t\t                   |___/      \n");
    printf("\t\t\t\tGame paused. Press 'Enter' to continue...\n");
    while (1) {
        char ch = getch();
        if (ch == 13) { // 13 is the ASCII code for Enter key
            break;
        }
    }
    system("cls"); // Clear the screen again to restore previous content

    // Restore the saved screen buffer
    WriteConsoleOutput(hStdout, buffer, bufferSize, coordScreen, &readRegion);
    free(buffer);

    // Move the cursor to the last position
    SetConsoleCursorPosition(hStdout, cursorPosition);
}

void ask_question(Question q, int *correct) {
    printf("\n%s\n", q.question);
    for (int i = 0; i < 4; i++) {
        printf("%c. %s\n", 'A' + i, q.options[i]);
    }
    printf("Choose from the following options: A, B, C, or D\n");
    char answer;
    timed_out = 0;

    // Countdown from 10 to 1
    for (int i = 10; i >= 0; i--) {
        printf("\rTime left: %02d\t", i); // Print the countdown with label and leading zeros
        fflush(stdout); // Flush the output buffer to ensure the digit is printed immediately
        sleep(1); // Pause for one second
        if (i == 0) {
            printf("\nTime over\n");
            timed_out = 1;
            *correct = 0;
            return;
        }
        if (kbhit()) { // Check if a key has been pressed
            char ch = getch();
            if (ch == 'X' || ch == 'x') { // Pause the game if 'X' is pressed
                pauseGame();
                printf("\rTime left: %02d\t", i); // Restore the timer display
                fflush(stdout);
                continue; // Continue the countdown
            }
            answer = toupper(ch); // Get the pressed key
            if (answer >= 'A' && answer <= 'D') {
                break;
            } else {
                printf("\nInvalid choice. Please choose from A, B, C, or D.\n");
            }
        }
    }

    // Move cursor to the next line for user input
    printf("\n\n");

    if (answer == q.correct_option) {
        *correct = 1;
    } else {
        *correct = 0;
    }
}

void load_words(WordEntry words[], int *num_words) {
    // Example words and questions
    strcpy(words[0].word, "programming");

    // C language basics questions
    strcpy(words[0].questions[0].question, "Which of the following is a valid C variable name?");
    strcpy(words[0].questions[0].options[0], "int");
    strcpy(words[0].questions[0].options[1], "main");
    strcpy(words[0].questions[0].options[2], "variable_name");
    strcpy(words[0].questions[0].options[3], "123variable");
    words[0].questions[0].correct_option = 'C';

    strcpy(words[0].questions[1].question, "Which of the following is used to declare a constant in C?");
    strcpy(words[0].questions[1].options[0], "#define");
    strcpy(words[0].questions[1].options[1], "const");
    strcpy(words[0].questions[1].options[2], "constant");
    strcpy(words[0].questions[1].options[3], "final");
    words[0].questions[1].correct_option = 'B';

    strcpy(words[0].questions[2].question, "Which of the following is not a valid storage class in C?");
    strcpy(words[0].questions[2].options[0], "auto");
    strcpy(words[0].questions[2].options[1], "register");
    strcpy(words[0].questions[2].options[2], "static");
    strcpy(words[0].questions[2].options[3], "volatile");
    words[0].questions[2].correct_option = 'D';

    strcpy(words[0].questions[3].question, "Which of the following is the correct syntax to declare a pointer?");
    strcpy(words[0].questions[3].options[0], "int *ptr;");
    strcpy(words[0].questions[3].options[1], "int ptr*;");
    strcpy(words[0].questions[3].options[2], "int &ptr;");
    strcpy(words[0].questions[3].options[3], "int ptr&;");
    words[0].questions[3].correct_option = 'A';

    strcpy(words[0].questions[4].question, "Which of the following is not a valid loop construct in C?");
    strcpy(words[0].questions[4].options[0], "for");
    strcpy(words[0].questions[4].options[1], "while");
    strcpy(words[0].questions[4].options[2], "do-while");
    strcpy(words[0].questions[4].options[3], "foreach");
    words[0].questions[4].correct_option = 'D';

    strcpy(words[0].questions[5].question, "Which of the following is used to allocate memory dynamically in C?");
    strcpy(words[0].questions[5].options[0], "malloc");
    strcpy(words[0].questions[5].options[1], "alloc");
    strcpy(words[0].questions[5].options[2], "new");
    strcpy(words[0].questions[5].options[3], "create");
    words[0].questions[5].correct_option = 'A';

    strcpy(words[0].questions[6].question, "Which of the following is not a valid data type in C?");
    strcpy(words[0].questions[6].options[0], "int");
    strcpy(words[0].questions[6].options[1], "float");
    strcpy(words[0].questions[6].options[2], "double");
    strcpy(words[0].questions[6].options[3], "real");
    words[0].questions[6].correct_option = 'D';

    strcpy(words[0].questions[7].question, "Which of the following is used to read a formatted input in C?");
    strcpy(words[0].questions[7].options[0], "scanf");
    strcpy(words[0].questions[7].options[1], "printf");
    strcpy(words[0].questions[7].options[2], "getchar");
    strcpy(words[0].questions[7].options[3], "gets");
    words[0].questions[7].correct_option = 'A';

    strcpy(words[0].questions[8].question, "Which of the following is used to terminate a loop in C?");
    strcpy(words[0].questions[8].options[0], "break");
    strcpy(words[0].questions[8].options[1], "exit");
    strcpy(words[0].questions[8].options[2], "terminate");
    strcpy(words[0].questions[8].options[3], "stop");
    words[0].questions[8].correct_option = 'A';

    strcpy(words[0].questions[9].question, "Which of the following is used to define a macro in C?");
    strcpy(words[0].questions[9].options[0], "#define");
    strcpy(words[0].questions[9].options[1], "macro");
    strcpy(words[0].questions[9].options[2], "const");
    strcpy(words[0].questions[9].options[3], "define");
    words[0].questions[9].correct_option = 'A';

    strcpy(words[0].questions[10].question, "Which of the following is used to include a library in C?");
    strcpy(words[0].questions[10].options[0], "#include");
    strcpy(words[0].questions[10].options[1], "import");
    strcpy(words[0].questions[10].options[2], "using");
    strcpy(words[0].questions[10].options[3], "library");
    words[0].questions[10].correct_option = 'A';

    strcpy(words[0].questions[11].question, "Which of the following is used to define a function in C?");
    strcpy(words[0].questions[11].options[0], "function");
    strcpy(words[0].questions[11].options[1], "def");
    strcpy(words[0].questions[11].options[2], "void");
    strcpy(words[0].questions[11].options[3], "func");
    words[0].questions[11].correct_option = 'C';

    strcpy(words[0].questions[12].question, "Which of the following is used to return a value from a function in C?");
    strcpy(words[0].questions[12].options[0], "return");
    strcpy(words[0].questions[12].options[1], "exit");
    strcpy(words[0].questions[12].options[2], "break");
    strcpy(words[0].questions[12].options[3], "stop");
    words[0].questions[12].correct_option = 'A';

    strcpy(words[0].questions[13].question, "Which of the following is used to declare a structure in C?");
    strcpy(words[0].questions[13].options[0], "struct");
    strcpy(words[0].questions[13].options[1], "class");
    strcpy(words[0].questions[13].options[2], "object");
    strcpy(words[0].questions[13].options[3], "structure");
    words[0].questions[13].correct_option = 'A';

    strcpy(words[0].questions[14].question, "What is the result of 3 + 2 * 2?");
    strcpy(words[0].questions[14].options[0], "7");
    strcpy(words[0].questions[14].options[1], "10");
    strcpy(words[0].questions[14].options[2], "8");
    strcpy(words[0].questions[14].options[3], "6");
    words[0].questions[14].correct_option = 'A';

    strcpy(words[0].questions[15].question, "What is the capital of France?");
    strcpy(words[0].questions[15].options[0], "Paris");
    strcpy(words[0].questions[15].options[1], "London");
    strcpy(words[0].questions[15].options[2], "Berlin");
    strcpy(words[0].questions[15].options[3], "Madrid");
    words[0].questions[15].correct_option = 'A';

    strcpy(words[0].questions[16].question, "Which planet is known as the Red Planet?");
    strcpy(words[0].questions[16].options[0], "Earth");
    strcpy(words[0].questions[16].options[1], "Mars");
    strcpy(words[0].questions[16].options[2], "Jupiter");
    strcpy(words[0].questions[16].options[3], "Saturn");
    words[0].questions[16].correct_option = 'B';

    strcpy(words[0].questions[17].question, "What is the capital of Canada?");
    strcpy(words[0].questions[17].options[0], "Toronto");
    strcpy(words[0].questions[17].options[1], "Vancouver");
    strcpy(words[0].questions[17].options[2], "Ottawa");
    strcpy(words[0].questions[17].options[3], "Montreal");
    words[0].questions[17].correct_option = 'C';

    strcpy(words[0].questions[18].question, "Who wrote 'To Kill a Mockingbird'?");
    strcpy(words[0].questions[18].options[0], "Harper Lee");
    strcpy(words[0].questions[18].options[1], "Mark Twain");
    strcpy(words[0].questions[18].options[2], "Ernest Hemingway");
    strcpy(words[0].questions[18].options[3], "F. Scott Fitzgerald");
    words[0].questions[18].correct_option = 'A';

    strcpy(words[0].questions[19].question, "What is the smallest prime number?");
    strcpy(words[0].questions[19].options[0], "1");
    strcpy(words[0].questions[19].options[1], "2");
    strcpy(words[0].questions[19].options[2], "3");
    strcpy(words[0].questions[19].options[3], "5");
    words[0].questions[19].correct_option = 'B';

    strcpy(words[0].questions[20].question, "What is the chemical symbol for gold?");
    strcpy(words[0].questions[20].options[0], "Au");
    strcpy(words[0].questions[20].options[1], "Ag");
    strcpy(words[0].questions[20].options[2], "Pb");
    strcpy(words[0].questions[20].options[3], "Fe");
    words[0].questions[20].correct_option = 'A';

    strcpy(words[0].questions[21].question, "Which language is used for web development?");
    strcpy(words[0].questions[21].options[0], "Python");
    strcpy(words[0].questions[21].options[1], "JavaScript");
    strcpy(words[0].questions[21].options[2], "C++");
    strcpy(words[0].questions[21].options[3], "Java");
    words[0].questions[21].correct_option = 'B';

    strcpy(words[0].questions[22].question, "What is the capital of Japan?");
    strcpy(words[0].questions[22].options[0], "Beijing");
    strcpy(words[0].questions[22].options[1], "Seoul");
    strcpy(words[0].questions[22].options[2], "Tokyo");
    strcpy(words[0].questions[22].options[3], "Bangkok");
    words[0].questions[22].correct_option = 'C';

    strcpy(words[0].questions[23].question, "Which element has the chemical symbol 'O'?");
    strcpy(words[0].questions[23].options[0], "Oxygen");
    strcpy(words[0].questions[23].options[1], "Gold");
    strcpy(words[0].questions[23].options[2], "Silver");
    strcpy(words[0].questions[23].options[3], "Iron");
    words[0].questions[23].correct_option = 'A';

    strcpy(words[0].questions[24].question, "What is the largest planet in our solar system?");
    strcpy(words[0].questions[24].options[0], "Earth");
    strcpy(words[0].questions[24].options[1], "Mars");
    strcpy(words[0].questions[24].options[2], "Jupiter");
    strcpy(words[0].questions[24].options[3], "Saturn");
    words[0].questions[24].correct_option = 'C';

    strcpy(words[0].questions[25].question, "Who painted the Mona Lisa?");
    strcpy(words[0].questions[25].options[0], "Vincent van Gogh");
    strcpy(words[0].questions[25].options[1], "Pablo Picasso");
    strcpy(words[0].questions[25].options[2], "Leonardo da Vinci");
    strcpy(words[0].questions[25].options[3], "Claude Monet");
    words[0].questions[25].correct_option = 'C';

    strcpy(words[0].questions[26].question, "What is the smallest country in the world?");
    strcpy(words[0].questions[26].options[0], "Monaco");
    strcpy(words[0].questions[26].options[1], "Vatican City");
    strcpy(words[0].questions[26].options[2], "San Marino");
    strcpy(words[0].questions[26].options[3], "Liechtenstein");
    words[0].questions[26].correct_option = 'B';

    strcpy(words[0].questions[27].question, "What is the square root of 64?");
    strcpy(words[0].questions[27].options[0], "6");
    strcpy(words[0].questions[27].options[1], "7");
    strcpy(words[0].questions[27].options[2], "8");
    strcpy(words[0].questions[27].options[3], "9");
    words[0].questions[27].correct_option = 'C';

    strcpy(words[0].questions[28].question, "Which gas is most abundant in the Earth's atmosphere?");
    strcpy(words[0].questions[28].options[0], "Oxygen");
    strcpy(words[0].questions[28].options[1], "Carbon Dioxide");
    strcpy(words[0].questions[28].options[2], "Nitrogen");
    strcpy(words[0].questions[28].options[3], "Hydrogen");
    words[0].questions[28].correct_option = 'C';

    strcpy(words[0].questions[29].question, "What is the longest river in the world?");
    strcpy(words[0].questions[29].options[0], "Amazon River");
    strcpy(words[0].questions[29].options[1], "Nile River");
    strcpy(words[0].questions[29].options[2], "Yangtze River");
    strcpy(words[0].questions[29].options[3], "Mississippi River");
    words[0].questions[29].correct_option = 'B';

    strcpy(words[0].questions[30].question, "Which of the following is a valid C function declaration?");
    strcpy(words[0].questions[30].options[0], "int func();");
    strcpy(words[0].questions[30].options[1], "func int();");
    strcpy(words[0].questions[30].options[2], "int func[];");
    strcpy(words[0].questions[30].options[3], "int func{};");
    words[0].questions[30].correct_option = 'A';

    strcpy(words[0].questions[31].question, "Which of the following is used to define a structure in C?");
    strcpy(words[0].questions[31].options[0], "struct");
    strcpy(words[0].questions[31].options[1], "class");
    strcpy(words[0].questions[31].options[2], "object");
    strcpy(words[0].questions[31].options[3], "structure");
    words[0].questions[31].correct_option = 'A';

    strcpy(words[0].questions[32].question, "Which of the following is used to return a value from a function in C?");
    strcpy(words[0].questions[32].options[0], "return");
    strcpy(words[0].questions[32].options[1], "exit");
    strcpy(words[0].questions[32].options[2], "break");
    strcpy(words[0].questions[32].options[3], "stop");
    words[0].questions[32].correct_option = 'A';

    strcpy(words[0].questions[33].question, "Which of the following is used to declare a pointer in C?");
    strcpy(words[0].questions[33].options[0], "int *ptr;");
    strcpy(words[0].questions[33].options[1], "int ptr*;");
    strcpy(words[0].questions[33].options[2], "int &ptr;");
    strcpy(words[0].questions[33].options[3], "int ptr&;");
    words[0].questions[33].correct_option = 'A';

    strcpy(words[0].questions[34].question, "Which of the following is used to allocate memory dynamically in C?");
    strcpy(words[0].questions[34].options[0], "malloc");
    strcpy(words[0].questions[34].options[1], "alloc");
    strcpy(words[0].questions[34].options[2], "new");
    strcpy(words[0].questions[34].options[3], "create");
    words[0].questions[34].correct_option = 'A';

    strcpy(words[0].questions[35].question, "Which of the following is used to read a formatted input in C?");
    strcpy(words[0].questions[35].options[0], "scanf");
    strcpy(words[0].questions[35].options[1], "printf");
    strcpy(words[0].questions[35].options[2], "getchar");
    strcpy(words[0].questions[35].options[3], "gets");
    words[0].questions[35].correct_option = 'A';

    strcpy(words[0].questions[36].question, "Which of the following is used to terminate a loop in C?");
    strcpy(words[0].questions[36].options[0], "break");
    strcpy(words[0].questions[36].options[1], "exit");
    strcpy(words[0].questions[36].options[2], "terminate");
    strcpy(words[0].questions[36].options[3], "stop");
    words[0].questions[36].correct_option = 'A';

    strcpy(words[0].questions[37].question, "Which of the following is used to define a macro in C?");
    strcpy(words[0].questions[37].options[0], "#define");
    strcpy(words[0].questions[37].options[1], "macro");
    strcpy(words[0].questions[37].options[2], "const");
    strcpy(words[0].questions[37].options[3], "define");
    words[0].questions[37].correct_option = 'A';

    strcpy(words[0].questions[38].question, "Which of the following is used to include a library in C?");
    strcpy(words[0].questions[38].options[0], "#include");
    strcpy(words[0].questions[38].options[1], "import");
    strcpy(words[0].questions[38].options[2], "using");
    strcpy(words[0].questions[38].options[3], "library");
    words[0].questions[38].correct_option = 'A';

    strcpy(words[0].questions[39].question, "Which of the following is used to declare a constant in C?");
    strcpy(words[0].questions[39].options[0], "#define");
    strcpy(words[0].questions[39].options[1], "const");
    strcpy(words[0].questions[39].options[2], "constant");
    strcpy(words[0].questions[39].options[3], "final");
    words[0].questions[39].correct_option = 'B';

    strcpy(words[0].questions[40].question, "What is the capital of Italy?");
    strcpy(words[0].questions[40].options[0], "Rome");
    strcpy(words[0].questions[40].options[1], "Paris");
    strcpy(words[0].questions[40].options[2], "Berlin");
    strcpy(words[0].questions[40].options[3], "Madrid");
    words[0].questions[40].correct_option = 'A';

    strcpy(words[0].questions[41].question, "Which planet is known as the Blue Planet?");
    strcpy(words[0].questions[41].options[0], "Mars");
    strcpy(words[0].questions[41].options[1], "Earth");
    strcpy(words[0].questions[41].options[2], "Jupiter");
    strcpy(words[0].questions[41].options[3], "Saturn");
    words[0].questions[41].correct_option = 'B';

    strcpy(words[0].questions[42].question, "What is the largest mammal?");
    strcpy(words[0].questions[42].options[0], "Elephant");
    strcpy(words[0].questions[42].options[1], "Blue Whale");
    strcpy(words[0].questions[42].options[2], "Giraffe");
    strcpy(words[0].questions[42].options[3], "Hippopotamus");
    words[0].questions[42].correct_option = 'B';

    strcpy(words[0].questions[43].question, "What is the smallest continent?");
    strcpy(words[0].questions[43].options[0], "Asia");
    strcpy(words[0].questions[43].options[1], "Europe");
    strcpy(words[0].questions[43].options[2], "Australia");
    strcpy(words[0].questions[43].options[3], "Antarctica");
    words[0].questions[43].correct_option = 'C';

    strcpy(words[0].questions[44].question, "What is the chemical symbol for water?");
    strcpy(words[0].questions[44].options[0], "H2O");
    strcpy(words[0].questions[44].options[1], "O2");
    strcpy(words[0].questions[44].options[2], "CO2");
    strcpy(words[0].questions[44].options[3], "NaCl");
    words[0].questions[44].correct_option = 'A';

    strcpy(words[0].questions[45].question, "What is the hardest natural substance on Earth?");
    strcpy(words[0].questions[45].options[0], "Gold");
    strcpy(words[0].questions[45].options[1], "Iron");
    strcpy(words[0].questions[45].options[2], "Diamond");
    strcpy(words[0].questions[45].options[3], "Platinum");
    words[0].questions[45].correct_option = 'C';

    strcpy(words[0].questions[46].question, "What is the tallest mountain in the world?");
    strcpy(words[0].questions[46].options[0], "K2");
    strcpy(words[0].questions[46].options[1], "Mount Everest");
    strcpy(words[0].questions[46].options[2], "Kangchenjunga");
    strcpy(words[0].questions[46].options[3], "Lhotse");
    words[0].questions[46].correct_option = 'B';

    strcpy(words[0].questions[47].question, "What is the largest desert in the world?");
    strcpy(words[0].questions[47].options[0], "Sahara Desert");
    strcpy(words[0].questions[47].options[1], "Arabian Desert");
    strcpy(words[0].questions[47].options[2], "Gobi Desert");
    strcpy(words[0].questions[47].options[3], "Antarctic Desert");
    words[0].questions[47].correct_option = 'D';

    strcpy(words[0].questions[48].question, "What is the fastest land animal?");
    strcpy(words[0].questions[48].options[0], "Lion");
    strcpy(words[0].questions[48].options[1], "Cheetah");
    strcpy(words[0].questions[48].options[2], "Leopard");
    strcpy(words[0].questions[48].options[3], "Tiger");
    words[0].questions[48].correct_option = 'B';

    strcpy(words[0].questions[49].question, "What is the smallest planet in our solar system?");
    strcpy(words[0].questions[49].options[0], "Earth");
    strcpy(words[0].questions[49].options[1], "Mars");
    strcpy(words[0].questions[49].options[2], "Mercury");
    strcpy(words[0].questions[49].options[3], "Venus");
    words[0].questions[49].correct_option = 'C';

    strcpy(words[0].questions[50].question, "What is the longest river in the world?");
    strcpy(words[0].questions[50].options[0], "Amazon River");
    strcpy(words[0].questions[50].options[1], "Nile River");
    strcpy(words[0].questions[50].options[2], "Yangtze River");
    strcpy(words[0].questions[50].options[3], "Mississippi River");
    words[0].questions[50].correct_option = 'B';

    *num_words = 1; // Update this as you add more words
}

void choose_word(WordEntry words[], int num_words, char *word, Question questions[]) {
    int index = rand() % num_words;
    strcpy(word, words[index].word);
    for (int i = 0; i < MAX_TRIES; i++) {
        questions[i] = words[index].questions[i];
    }
    shuffle_questions(questions, MAX_TRIES); // Shuffle questions here
}

void shuffle_questions(Question questions[], int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Question temp = questions[i];
        questions[i] = questions[j];
        questions[j] = temp;
    }
}

void provide_hint(Question q) {
    printf("\n********** HINT **********\n");
    printf("Hint: The correct option is one of the following: ");
    for (int i = 0; i < 4; i++) {
        if (q.correct_option == 'A' + i) {
            printf("%c. %s\n", 'A' + i, q.options[i]);
            break;
        }
    }
    printf("**************************\n");
}

void play_game(char *word, Question questions[], int difficulty) {
    int word_len = strlen(word);
    char guessed[word_len + 1];
    for (int i = 0; i < word_len; i++) guessed[i] = '_';
    guessed[word_len] = '\0';

    int tries = 0;
    int score = 0;
    int max_tries = MAX_TRIES; // Full hangman stages till 10
    int correct_streak = 0; // Track the number of consecutive correct answers
    int incorrect_streak = 0; // Track the number of consecutive incorrect answers

    shuffle_questions(questions, MAX_TRIES);

    int asked_questions[MAX_TRIES];
    for (int i = 0; i < MAX_TRIES; i++) asked_questions[i] = -1; // Initialize to -1
    int questions_asked = 0; // Counter for the number of questions asked

    while (tries < max_tries) {
        printf("\nWord: %s\n", guessed);
        print_hangman(tries);

        int correct = 0;
        int question_index;

        // Find the next unasked question
        for (question_index = 0; question_index < MAX_TRIES; question_index++) {
            if (asked_questions[question_index] == -1) {
                break;
            }
        }

        asked_questions[question_index] = 1; // Mark the question as asked
        questions_asked++;

        // Provide hint for the current question if there were three consecutive wrong answers
        if (incorrect_streak == 3) {
            provide_hint(questions[question_index]);
            incorrect_streak = 0; // Reset the streak after providing a hint
        }

        ask_question(questions[question_index], &correct);

        if (timed_out) {
            tries++;
            correct_streak = 0;
            incorrect_streak++;
            printf("Incorrect streak: %d\n", incorrect_streak);
        } else if (correct) {
            printf("Correct answer!\n");
            for (int i = 0; i < word_len; i++) {
                if (guessed[i] == '_') {
                    guessed[i] = word[i];
                    break;
                }
            }
            score += 10; // Increase score for correct answer
            correct_streak++;
            incorrect_streak = 0; // Reset the streak on a correct answer
        } else {
            printf("Wrong answer!\n");
            tries++;
            correct_streak = 0; // Reset the streak on a wrong answer
            incorrect_streak++;
            printf("Incorrect streak: %d\n", incorrect_streak); // Debug print
        }

        // Reset the asked questions array if all questions have been asked
        if (questions_asked == MAX_TRIES) {
            for (int i = 0; i < MAX_TRIES; i++) asked_questions[i] = -1; // Reset to -1
            questions_asked = 0;
            shuffle_questions(questions, MAX_TRIES); // Shuffle questions again
        }

        if (strcmp(word, guessed) == 0) {
            print_winning_screen(word, score);
            playSound("Applause audio.wav"); // Play applause sound on winning
            return;
        }
    }

    if (tries == max_tries) {
        print_hangman(tries);
        print_game_over_screen(word);
        printf("Your score: %d\n", score);
        playSound("loser audio.wav"); // Play loser sound on losing
    }
}

void print_welcome_screen() {
    printf("*********************************\n");
    printf("*                               *\n");
    printf("*       Welcome to Hangman      *\n");
    printf("*                               *\n");
    printf("*********************************\n");
    printf("  +---+\n");
    printf("  |   |\n");
    printf("      |\n");
    printf("      |\n");
    printf("      |\n");
    printf("      |\n");
    printf("=========\n");
}

void print_game_over_screen(const char* word) {
    printf("\n*********************************\n");
    printf("*                               *\n");
    printf("*          GAME OVER!           *\n");
    printf("*                               *\n");
    printf("* The word was: %s              *\n", word);
    printf("*                               *\n");
    printf("*********************************\n");
    printf("  +---+\n");
    printf("  |   |\n");
    printf(" [O] |\n");
    printf(" /|\\  |\n");
    printf(" / \\  |\n");
    printf("      |\n");
    printf("=========\n");
}

void print_winning_screen(const char* word, int score) {
    printf("\n***************************************\n");
    printf("*                                     *\n");
    printf("*         Congratulations!            *\n");
    printf("*                                     *\n");
    printf("* You guessed the word: %-12s *\n", word); 
    printf("* Your score: %-22d *\n", score); 
    printf("*                                     *\n");
    printf("***************************************\n");
    printf("  +---+\n");
    printf("  |   |\n");
    printf("      |\n");
    printf(" \\O/  |\n");
    printf("  |   |\n");
    printf(" / \\  |\n");
    printf("=========\n");
}
