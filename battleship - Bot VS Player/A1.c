#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define GRID_SIZE 10
#define MAX_RADAR_USES 3
#define MAX_SMOKE_SCREENS 2

// Keeps track of placed ships on grid1
char placedShips[6] = "NNNNN";

// Ship structure to define each ship type
typedef struct {
    char type;    // Type of ship (e.g., 'C' for Carrier)
    int size;     // Size of the ship
    int hits;     // Tracks hits for each ship
} Ship;

// Fleet structure to define a collection of ships for each player
typedef struct {
    Ship carrier;
    Ship battleship;
    Ship cruiser;
    Ship submarine;
    Ship destroyer;
    int shipsDestroyed;
    int artilleryUnlocked;
    int torpedoUnlocked;
} Fleet;

// Smoke screen structure
typedef struct {
    int x, y;             // Coordinates of the smoke screen's top-left corner
    int turnsRemaining;   // Turns remaining before the smoke expires
} SmokeScreen;

// Semi-Heuristic AI for easy mode
typedef struct {
    int x, y;         // Coordinates of the last hit
    int isHit;        // Indicates if the AI has hit a ship
    int direction;    // 0 for vertical, 1 for horizontal, -1 if no direction
} AITarget;

// Function prototypes
void initGrid(char grid[GRID_SIZE][GRID_SIZE]);
void printGrid(char grid[GRID_SIZE][GRID_SIZE]);
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]);
void initializeFleet(Fleet *fleet);
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation);
void Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y);
void Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y);
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int col);
void RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses, SmokeScreen smokes[], int activeSmokeCount);
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y);
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount);
void initAITarget(AITarget *aiTarget);
void aiTurn(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, AITarget *aiTarget);

// Initializes the grid with water '~'
void initGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '~';
        }
    }
}

// Prints the grid with column and row headers
void printGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    printf("   A B C D E F G H I J\n");
    for (int i = 0; i < GRID_SIZE; i++) {
        printf("%2d ", i + 1);
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
}

// Displays grids for the current player and their view of the opponent's grid
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]) {
    if (turn == 1) {
        printf("Your Grid (Player 1):\n");
        printGrid(grid1);
        printf("Opponent's Visible Grid:\n");
        printGrid(grid2for1);
    } else {
        printf("Your Grid (Player 2):\n");
        printGrid(grid2);
        printf("Opponent's Visible Grid:\n");
        printGrid(grid1for2);
    }
}

// Initializes the fleet with default ships
void initializeFleet(Fleet *fleet) {
    fleet->carrier = (Ship){'C', 5, 0};
    fleet->battleship = (Ship){'B', 4, 0};
    fleet->cruiser = (Ship){'c', 3, 0};
    fleet->submarine = (Ship){'s', 3, 0};
    fleet->destroyer = (Ship){'d', 2, 0};
    fleet->shipsDestroyed = 0;
    fleet->artilleryUnlocked = 0;
    fleet->torpedoUnlocked = 0;
}

// Places a ship on the grid
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation) {
    for (int i = 0; i < ship->size; i++) {
        if (orientation == 0 && (y + i >= GRID_SIZE || grid[x][y + i] != '~')) return -1;
        if (orientation == 1 && (x + i >= GRID_SIZE || grid[x + i][y] != '~')) return -1;
    }
    for (int i = 0; i < ship->size; i++) {
        if (orientation == 0) grid[x][y + i] = ship->type;
        if (orientation == 1) grid[x + i][y] = ship->type;
    }
    return 0;
}

// Fires at a specific location
void Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y) {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        printf("Invalid coordinates!\n");
        return;
    }
    if (grid[x][y] == '~') {
        grid[x][y] = 'm';  // Miss
        printf("Missed!\n");
    } else if (grid[x][y] != 'm' && grid[x][y] != 'h') {
        char shipType = grid[x][y];
        grid[x][y] = 'h';  // Hit
        printf("Hit!\n");

        int shipDestroyed = 0;
        switch (shipType) {
            case 'C':
                fleet->carrier.hits++;
                if (fleet->carrier.hits == fleet->carrier.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyed = 1;
                }
                break;
            case 'B':
                fleet->battleship.hits++;
                if (fleet->battleship.hits == fleet->battleship.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyed = 1;
                }
                break;
            case 'c':
                fleet->cruiser.hits++;
                if (fleet->cruiser.hits == fleet->cruiser.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyed = 1;
                }
                break;
            case 's':
                fleet->submarine.hits++;
                if (fleet->submarine.hits == fleet->submarine.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyed = 1;
                }
                break;
            case 'd':
                fleet->destroyer.hits++;
                if (fleet->destroyer.hits == fleet->destroyer.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyed = 1;
                }
                break;
        }

        if (shipDestroyed) {
            printf("A ship has been destroyed!\n");

            if (!fleet->artilleryUnlocked && fleet->shipsDestroyed >= 1) {
                fleet->artilleryUnlocked = 1;
                printf("Artillery has been unlocked!\n");
            }

            if (!fleet->torpedoUnlocked && fleet->shipsDestroyed >= 3) {
                fleet->torpedoUnlocked = 1;
                printf("Torpedo has been unlocked!\n");
            }
        }
    } else {
        printf("Already fired here!\n");
    }
}

// Executes an artillery strike
void Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y) {
    if (fleet->artilleryUnlocked) {
        printf("Artillery attack at %c%d!\n", 'A' + y, x + 1);
        for (int i = x; i < x + 2 && i < GRID_SIZE; i++) {
            for (int j = y; j < y + 2 && j < GRID_SIZE; j++) {
                Fire(grid, fleet, i, j);
            }
        }
        fleet->artilleryUnlocked = 0; // Reset artillery availability
    } else {
        printf("Artillery is not unlocked yet!\n");
    }
}

// Executes a torpedo strike on a column
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int col) {
    if (fleet->torpedoUnlocked) {
        printf("Torpedo attack on column %c!\n", col + 'A');
        for (int i = 0; i < GRID_SIZE; i++) {
            Fire(grid, fleet, i, col);
        }
        fleet->torpedoUnlocked = 0; // Reset torpedo availability
    } else {
        printf("Torpedo is not unlocked yet!\n");
    }
}

// Radar sweep implementation
void RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses, SmokeScreen smokes[], int activeSmokeCount) {
    for (int i = 0; i < activeSmokeCount; i++) {
        if (x >= smokes[i].x && x < smokes[i].x + 2 && y >= smokes[i].y && y < smokes[i].y + 2) {
            printf("Radar blocked by smoke.\n");
            return;
        }
    }

    if (*radarUses >= MAX_RADAR_USES) {
        printf("No more radar sweeps available.\n");
        return;
    }

    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        printf("Invalid coordinates for radar.\n");
        return;
    }

    int enemyFound = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (grid[x + i][y + j] != '~' && grid[x + i][y + j] != 'h' && grid[x + i][y + j] != 'm') {
                enemyFound = 1;
            }
        }
    }

    (*radarUses)++;
    if (enemyFound) {
        printf("Enemy ships found.\n");
    } else {
        printf("No enemy ships found.\n");
    }
}

// Smoke screen deployment
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y) {
     if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        printf("Invalid coordinates for smoke.\n");
        return;
    }

    if (*activeSmokeCount >= MAX_SMOKE_SCREENS) {
        printf("Maximum smoke screens deployed.\n");
        return;
    }

    smokes[*activeSmokeCount].x = x;
    smokes[*activeSmokeCount].y = y;
    smokes[*activeSmokeCount].turnsRemaining = 3;
    (*activeSmokeCount)++;

    printf("Smoke screen deployed at %c%d.\n", 'A' + y, x + 1);
}

// Update smoke screens
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount) {
    for (int i = 0; i < *activeSmokeCount; i++) {
        smokes[i].turnsRemaining--;
        if (smokes[i].turnsRemaining <= 0) {
            printf("Smoke screen at %c%d expired.\n", 'A' + smokes[i].y, smokes[i].x + 1);
            for (int j = i; j < *activeSmokeCount - 1; j++) {
                smokes[j] = smokes[j + 1];
            }
            (*activeSmokeCount)--;
            i--;
        }
    }
}

// Initializes AI target
void initAITarget(AITarget *aiTarget) {
    aiTarget->isHit = 0;
    aiTarget->direction = -1;
}

// AI turn logic
void aiTurn(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, AITarget *aiTarget) {
    int x, y;
    if (aiTarget->isHit) {
        x = aiTarget->x + (aiTarget->direction == 0 ? 1 : 0);
        y = aiTarget->y + (aiTarget->direction == 1 ? 1 : 0);
        if (x < GRID_SIZE && y < GRID_SIZE && grid[x][y] == '~') {
            Fire(grid, fleet, x, y);
            return;
        }
    }

    do {
        x = rand() % GRID_SIZE;
        y = rand() % GRID_SIZE;
    } while (grid[x][y] == 'm' || grid[x][y] == 'h');

    Fire(grid, fleet, x, y);
    printf("AI fires at %c%d.\n", 'A' + y, x + 1);

    if (grid[x][y] == 'h') {
        aiTarget->isHit = 1;
        aiTarget->x = x;
        aiTarget->y = y;
        aiTarget->direction = rand() % 2;
    }
}

int main() {
    char grid1[GRID_SIZE][GRID_SIZE];
    char grid2[GRID_SIZE][GRID_SIZE];
    char grid1for2[GRID_SIZE][GRID_SIZE];
    char grid2for1[GRID_SIZE][GRID_SIZE];

    Fleet fleet1, fleet2;
    SmokeScreen smokes1[MAX_SMOKE_SCREENS];
    SmokeScreen smokes2[MAX_SMOKE_SCREENS];
    int activeSmokeCount1 = 0, activeSmokeCount2 = 0;
    int radarUses1 = 0, radarUses2 = 0;

    initializeFleet(&fleet1);
    initializeFleet(&fleet2);

    initGrid(grid1);
    initGrid(grid2);
    initGrid(grid1for2);
    initGrid(grid2for1);

    // Place ships for both players
    placeShip(grid1, &fleet1.carrier, 0, 0, 0);
    placeShip(grid1, &fleet1.battleship, 2, 0, 1);
    placeShip(grid1, &fleet1.cruiser, 4, 4, 0);
    placeShip(grid1, &fleet1.submarine, 6, 6, 1);
    placeShip(grid1, &fleet1.destroyer, 8, 3, 0);

    placeShip(grid2, &fleet2.carrier, 1, 1, 0);
    placeShip(grid2, &fleet2.battleship, 3, 1, 1);
    placeShip(grid2, &fleet2.cruiser, 5, 5, 0);
    placeShip(grid2, &fleet2.submarine, 7, 7, 1);
    placeShip(grid2, &fleet2.destroyer, 9, 4, 0);

    AITarget aiTarget1, aiTarget2;
    initAITarget(&aiTarget1);
    initAITarget(&aiTarget2);

    srand(time(NULL));
    int turn = rand() % 2 + 1;

    while (fleet1.shipsDestroyed != 5 && fleet2.shipsDestroyed != 5) {
        char input[32] = {0};
        char command[16] = {0};
        char target[5] = {0};
        int validCommand = 0;

        Fleet *opponentFleet = (turn == 1) ? &fleet2 : &fleet1;
        char (*oppositeGrid)[GRID_SIZE] = (turn == 1) ? grid2 : grid1;
        char (*playerGrid)[GRID_SIZE] = (turn == 1) ? grid1 : grid2;
        char (*opponentVisibleGrid)[GRID_SIZE] = (turn == 1) ? grid2for1 : grid1for2;
        SmokeScreen *activeSmokes = (turn == 1) ? smokes2 : smokes1;
        int *activeSmokeCount = (turn == 1) ? &activeSmokeCount2 : &activeSmokeCount1;
        int *radarUses = (turn == 1) ? &radarUses1 : &radarUses2;

        displayGrids(turn, grid1, grid2, grid1for2, grid2for1);

        if (turn == 2) {
            aiTurn(oppositeGrid, opponentFleet, turn == 1 ? &aiTarget2 : &aiTarget1);
            UpdateSmokeScreens(activeSmokes, activeSmokeCount);
            turn = 1;
            continue;
        }

        while (!validCommand) {
            printf("Player %d, enter your command (e.g., Fire A5, Radar A5, Smoke A5): ", turn);
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0';
            
            // the C null terminator occupies last sport in target
            if (sscanf(input, "%15s %4s", command, target) < 2) {
                printf("Invalid input format! Try again.\n");
                continue;
            }

            if (strcmp(command, "Fire") == 0 || strcmp(command, "Artillery") == 0) {
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]) && isdigit(target[1])) {

                    char col;
                    int row;
                    if (sscanf(target, "%c%d", &col, &row) == 2) {
                        col = toupper(col);
                    } else {
                        printf("Invalid target format. Try again.\n");
                    }

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        validCommand = 1;
                        if (strcmp(command, "Fire") == 0) {
                            Fire(oppositeGrid, opponentFleet, row - 1, col - 'A');
                        } else if (opponentFleet->artilleryUnlocked) {
                            Artillery(oppositeGrid, opponentFleet, row - 1, col - 'A');
                        } else {
                            printf("Artillery is not unlocked yet!\n");
                        }
                    } else {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                } else {
                    printf("Invalid target! Use a format like 'A5'.\n");
                }
            } else if (strcmp(command, "Radar") == 0) {
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]) && isdigit(target[1])) {

                    char col;
                    int row;
                    if (sscanf(target, "%c%d", &col, &row) == 2) {
                        col = toupper(col);
                    } else {
                        printf("Invalid target format. Try again.\n");
                    }

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        validCommand = 1;
                        RadarSweep(oppositeGrid, 0, row - 1, col - 'A', radarUses, activeSmokes, *activeSmokeCount);
                    } else {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                } else {
                    printf("Invalid target! Use a format like 'A5'.\n");
                }
            } else if (strcmp(command, "Smoke") == 0) {
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]) && isdigit(target[1])) {

                    char col;
                    int row;
                    if (sscanf(target, "%c%d", &col, &row) == 2) {
                        col = toupper(col);
                    } else {
                        printf("Invalid target format. Try again.\n");
                    }

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        validCommand = 1;
                        DeploySmokeScreen(activeSmokes, activeSmokeCount, row - 1, col - 'A');
                    } else {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                } else {
                    printf("Invalid target! Use a format like 'A5'.\n");
                }
            } else {
                printf("Unknown command '%s'. Available commands: Fire, Artillery, Radar, Smoke.\n", command);
            }
        }

        UpdateSmokeScreens(activeSmokes, activeSmokeCount);
        turn = (turn == 1) ? 2 : 1;
    }

    if (fleet1.shipsDestroyed == 5) {
        printf("Player 2 wins!\n");
    } else {
        printf("Player 1 wins!\n");
    }

    return 0;
}