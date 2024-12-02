#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>

#define GRID_SIZE 10
#define MAX_RADAR_USES 3
#define MAX_SMOKE_SCREENS 2
#define DEBUG_MODE 0 

#define RESET_COLOR  "\x1b[0m"
#define BLUE_COLOR   "\x1b[34m"
#define RED_COLOR    "\x1b[31m"
#define GREEN_COLOR  "\x1b[32m"

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
    int artilleryUsed;
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
} AITargetEasy;

// AI Targeting structure for hard difficulty
typedef struct {
    int x, y;             // Coordinates of the initial hit
    int isHit;            // Indicates if the AI has a confirmed hit
    int direction;        // Direction for targeting: 0-North, 1-South, 2-East, 3-West, -1 if undefined
    int radarUsed;        // Indicates if Radar Sweep has been used for this hit
    int referenceX;       // Current reference X coordinate for following the ship
    int referenceY;       // Current reference Y coordinate for following the ship
    int caseNum;          // Current case number (1, 2, or 3)
    int directionIndex;   // For case 3, index into the direction sequence
} aiTargetHard;

// Function prototypes
void initGrid(char grid[GRID_SIZE][GRID_SIZE]);
void printGrid(char grid[GRID_SIZE][GRID_SIZE]);
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]);
void initializeFleet(Fleet *fleet);
int placeshipForAI(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation);
void Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y, int *shipDestroyed);
void Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y);
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int index, char axis);
void RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses,
                SmokeScreen smokes[], int activeSmokeCount, int *enemyFound);
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y,  Fleet *fleet);
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount);

//For easy AI
void initAITargetEasy(AITargetEasy *AITargetEasy);
void aiTurnEasy(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, AITargetEasy *AITargetEasy);
// For Hard AI
void initaiTargetHard(aiTargetHard *aiTargetHard);
void aiTurnHard(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, aiTargetHard *aiTargetHard,
            int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips, int *radarUses);

// Heatmap functions
void initHeatmap(int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips);
void updateHeatmapOnHit(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                        int numShips, int shipDestroyed, char opponentGrid[GRID_SIZE][GRID_SIZE]);
void updateHeatmapOnMiss(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                         int numShips);
void printHeatmap(int heatmap[GRID_SIZE][GRID_SIZE]);

// Helper function to get maximum of two integers
int max(int a, int b) {
    return (a > b) ? a : b;
}

void UpdateOppositeGrid(char opponentGrid[GRID_SIZE][GRID_SIZE], char playerView[GRID_SIZE][GRID_SIZE], char difficulty);
void printIntro();

void printIntro() {
    printf(BLUE_COLOR);
    printf("          _______________________\n");
    printf("   ______/    |    |    |    |   \\_________\n");
    printf("  /     |     |    |    |    |        |    \\\n");
    printf(" /______|_____|____|____|____|________|_____\\\n");
    printf(" \\                                         / \n");
    printf("  \\_______________________________________/\n");
    printf("      |  __   __   __   __   __   __   __  |\n");
    printf("      | |__| |__| |__| |__| |__| |__| |__| |\n");
    printf("     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

    printf(RED_COLOR);
    printf(" _           _   _   _       _____ _     _\n");      
    printf("| |         | | | | | |     / ____| |   (_)\n");      
    printf("| |__   __ _| |_| |_| | ___| (___ | |__  _ _ __ \n"); 
    printf("| '_ \\ / _` | __| __| |/ _ \\___ \\ | '_ \\| | '_ \\ \n");
    printf("| |_) | (_| | |_| |_| |  __/____) | | | | | |_) |\n");
    printf("|_.__/ \\__,_|\\__|\\__|_|\\___|_____/|_| |_|_| .__/ \n");
    printf("                                            | |  \n");  
    printf("                                            |_|  \n");  

    printf(GREEN_COLOR);
    printf("\nWelcome to BattleShip!\n");
    printf(RESET_COLOR);
}


// For updating the opposite grid after firing, takes into consideration easy and hard tracking difficulties
void UpdateOppositeGrid(char opponentGrid[GRID_SIZE][GRID_SIZE], char playerView[GRID_SIZE][GRID_SIZE], char difficulty) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (opponentGrid[i][j] == 'h') { // Update hits regardless of difficulty
                playerView[i][j] = 'h';
            } else if (difficulty == 'E' && opponentGrid[i][j] == 'm') { // Update misses only in easy mode
                playerView[i][j] = 'm';
            }
        }
    }
}

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
        printf("The opponent's grid:\n");
        printGrid(grid2for1);
    } else {
        printf("AI's Grid (Player 2):\n");
        printGrid(grid2);
        printf("What the AI can see:\n");
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
    fleet->artilleryUsed = 0;
}

// Places a ship on the grid
int placeshipForAI(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation) {
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

int isPlaced(Ship *ship){
    int j = 0;
    for(int i = 0; i < 6; i++){
        if(placedShips[i] != 'N')
            j++;
        else
            break;
    }

    for(int i = 0; i < 6; i++){
        if(placedShips[i] == ship->type){
            printf("This ship has already been placed on the grid.\n");
            return 1;
        }
    }
    placedShips[j] = ship->type;
    return 0;
}

// Place a ship on the grid based on coordinates and orientation
int placeShipForPlayer(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation) {
    // Check bounds and availability of cells
    if (orientation == 0) {  // Horizontal placement
        if (y + ship->size > GRID_SIZE) return -1;  // Out of bounds
        for (int i = 0; i < ship->size; i++) {
            if (grid[x][y + i] != '~') return -1;  // Spot occupied
        }
        if (isPlaced(ship) == 1) 
            return -1; // ship has already been placed
        for (int i = 0; i < ship->size; i++) {
            grid[x][y + i] = ship->type;
        }
    } else if (orientation == 1) {  // Vertical placement
        if (x + ship->size > GRID_SIZE) return -1;  // Out of bounds
        for (int i = 0; i < ship->size; i++) {
            if (grid[x + i][y] != '~') return -1;  // Spot occupied
        }
        if (isPlaced(ship) == 1) 
            return -1; // ship has already been placed
        for (int i = 0; i < ship->size; i++) {
            grid[x + i][y] = ship->type;
        }
    } else {
        return -1;  // Invalid orientation
    }
    return 0;  // Successful placement
}

// Fires at a specific location
void Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int x, int y, int *shipDestroyed) {
    *shipDestroyed = 0; // Initialize to no ship destroyed

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

        int shipDestroyedLocal = 0;
        switch (shipType) {
            case 'C':
                fleet->carrier.hits++;
                if (fleet->carrier.hits == fleet->carrier.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 'B':
                fleet->battleship.hits++;
                if (fleet->battleship.hits == fleet->battleship.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 'c':
                fleet->cruiser.hits++;
                if (fleet->cruiser.hits == fleet->cruiser.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 's':
                fleet->submarine.hits++;
                if (fleet->submarine.hits == fleet->submarine.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
            case 'd':
                fleet->destroyer.hits++;
                if (fleet->destroyer.hits == fleet->destroyer.size) {
                    fleet->shipsDestroyed++;
                    shipDestroyedLocal = 1;
                }
                break;
        }

        if (shipDestroyedLocal) {
            printf("A ship has been destroyed!\n");
            *shipDestroyed = 1; // Notify that a ship has been destroyed

            if (!fleet->artilleryUnlocked && fleet->shipsDestroyed >= 1) {
                if (fleet->artilleryUsed == 0) {
                    fleet->artilleryUnlocked = 1;
                    printf("Artillery has been unlocked!\n");
                    fleet->artilleryUsed = 1;
                }
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
        int shipDestroyed;
        for (int i = x; i < x + 2 && i < GRID_SIZE; i++) {
            for (int j = y; j < y + 2 && j < GRID_SIZE; j++) {
                Fire(grid, fleet, i, j,&shipDestroyed);
            }
        }
        fleet->artilleryUnlocked = 0; // Reset artillery availability
    } else {
        printf("Artillery is not unlocked yet!\n");
    }
}

// Executes a torpedo strike on a column
void Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, int index, char axis) {
    if (fleet->torpedoUnlocked) {
        int shipDestroyed;
        if (axis == 'C') {
            printf("Torpedo attack on column %c!\n", index + 'A');
            for (int i = 0; i < GRID_SIZE; i++) {
                Fire(grid, fleet, i, index, &shipDestroyed);
            }
        } else if (axis == 'R') {
            printf("Torpedo attack on row %d!\n", index + 1);
            for (int j = 0; j < GRID_SIZE; j++) {
                Fire(grid, fleet, j, index, &shipDestroyed);
            }
        }
        fleet->torpedoUnlocked = 0; // Reset torpedo availability
    } else {
        printf("Torpedo is not unlocked yet!\n");
    }
}

// Radar sweep implementation
void RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses,
                SmokeScreen smokes[], int activeSmokeCount, int *enemyFound) {
    for (int i = 0; i < activeSmokeCount; i++) {
        if (x >= smokes[i].x && x < smokes[i].x + 2 && y >= smokes[i].y && y < smokes[i].y + 2) {
            printf("Radar blocked by smoke.\n");
            *enemyFound = 0;
            return;
        }
    }

    if (*radarUses >= MAX_RADAR_USES) {
        printf("No more radar sweeps available.\n");
        *enemyFound = 0;
        return;
    }

    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        printf("Invalid coordinates for radar.\n");
        *enemyFound = 0;
        return;
    }

    *enemyFound = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (grid[x + i][y + j] != '~' && grid[x + i][y + j] != 'h' && grid[x + i][y + j] != 'm') {
                *enemyFound = 1;
            }
        }
    }

    (*radarUses)++;
    if (*enemyFound) {
        printf("Enemy ships found.\n");
    } else {
        printf("No enemy ships found.\n");
    }
}

// Deploys a smoke screen
void DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y, Fleet *fleet)
{
    // Check if smoke screens are unlocked
    int allowedSmokes = fleet->shipsDestroyed; // Allowed smokes based on sunk ships

    // Check if the AI has exceeded their allowed smoke screens
    if (*activeSmokeCount >= allowedSmokes)
    {
        printf("No available smoke screens.\n");
        return; // AI loses their turn
    }

    // Validate coordinates
    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1)
    {
        printf("Invalid coordinates for smoke screen deployment.\n");
        return;
    }

    // Deploy the smoke screen
    smokes[*activeSmokeCount].x = x;
    smokes[*activeSmokeCount].y = y;
    smokes[*activeSmokeCount].turnsRemaining = 3;
    (*activeSmokeCount)++;

    printf("Smoke screen deployed at %c%d.\n", 'A' + y, x + 1);
}

// Updates smoke screens by decrementing their remaining turns
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount)
{
    for (int i = 0; i < *activeSmokeCount; i++)
    {
        smokes[i].turnsRemaining--;
        if (smokes[i].turnsRemaining <= 0)
        {
            if (DEBUG_MODE)
                printf("Smoke screen at %c%d has expired.\n", 'A' + smokes[i].y,
                       smokes[i].x + 1);
            // Remove the smoke screen by shifting the array
            for (int j = i; j < *activeSmokeCount - 1; j++)
            {
                smokes[j] = smokes[j + 1];
            }
            (*activeSmokeCount)--;
            i--; // Adjust index after removal
        }
    }
}

// Initializes AI target
void initAITargetEasy(AITargetEasy *AITargetEasy) {
    AITargetEasy->isHit = 0;
    AITargetEasy->direction = -1;
}

// AI turn logic
void aiTurnEasy(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, AITargetEasy *AITargetEasy) {
    int x, y;
    if (AITargetEasy->isHit) {
        int shipsDestroyed;
        x = AITargetEasy->x + (AITargetEasy->direction == 0 ? 1 : 0);
        y = AITargetEasy->y + (AITargetEasy->direction == 1 ? 1 : 0);
        if (x < GRID_SIZE && y < GRID_SIZE && grid[x][y] == '~') {
            Fire(grid, fleet, x, y, &shipsDestroyed);
            return;
        }
    }

    do {
        x = rand() % GRID_SIZE;
        y = rand() % GRID_SIZE;
    } while (grid[x][y] == 'm' || grid[x][y] == 'h');
    int shipsDestroyed;
    Fire(grid, fleet, x, y,&shipsDestroyed);
    printf("AI fires at %c%d.\n", 'A' + y, x + 1);

    if (grid[x][y] == 'h') {
        AITargetEasy->isHit = 1;
        AITargetEasy->x = x;
        AITargetEasy->y = y;
        AITargetEasy->direction = rand() % 2;
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



    AITargetEasy AITargetEasy1, AITargetEasy2;
    initAITargetEasy(&AITargetEasy1);
    initAITargetEasy(&AITargetEasy2);

    aiTargetHard aiTargetHard1, aiTargetHard2;
    initaiTargetHard(&aiTargetHard1);
    initaiTargetHard(&aiTargetHard2);

    // Initialize heatmaps
    int shipSizes[] = {5, 4, 3, 3, 2};
    int numShips = 5;
    int heatmap1[GRID_SIZE][GRID_SIZE];
    int heatmap2[GRID_SIZE][GRID_SIZE];
    initHeatmap(heatmap1, shipSizes, numShips);
    initHeatmap(heatmap2, shipSizes, numShips);

    printIntro();
    
    char playerName1[50], playerName2[50];
    printf("Enter Player 1's name: ");
    fgets(playerName1, sizeof(playerName1), stdin);
    playerName1[strcspn(playerName1, "\n")] = '\0'; 

    char gameMode;
    do {
        printf("%s, do you want to play against the AI or in a 1v1 game? (A)I or (1)1: ", playerName1);
        scanf(" %c", &gameMode);
        gameMode = toupper(gameMode);
    } while (gameMode != 'A' && gameMode != '1');

        int x, y, orientation;
        char shipType;
        Ship *ship;
    if (gameMode = 'A'){
        // Place ships statically for the AI
        placeshipForAI(grid2, &fleet2.carrier, 1, 1, 0);
        placeshipForAI(grid2, &fleet2.battleship, 3, 1, 1);
        placeshipForAI(grid2, &fleet2.cruiser, 5, 5, 0);
        placeshipForAI(grid2, &fleet2.submarine, 7, 7, 1);
        placeshipForAI(grid2, &fleet2.destroyer, 9, 4, 0);

        printf("\n%s, it is your turn to place your ships on the grid: \n", playerName1);
        printf("Enter ship type (C for Carrier, B for Battleship, c for Cruiser, s for Submarine, d for Destroyer), coordinates (row and column), and orientation (0 for horizontal, 1 for vertical):\n");

        for (int i = 0; i < 5; i++) {  // 5 ships per player
            printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
            scanf(" %c %d %d %d", &shipType, &x, &y, &orientation);
            x--;  // Convert to 0-based index
            y--;

            // Determine which ship to place
            switch (shipType) {
                case 'C': ship = &fleet1.carrier; break;
                case 'B': ship = &fleet1.battleship; break;
                case 'c': ship = &fleet1.cruiser; break;
                case 's': ship = &fleet1.submarine; break;
                case 'd': ship = &fleet1.destroyer; break;
                default: 
                    printf("Invalid ship type. Try again.\n");
                    i--;  // Repeat this step
                    continue;
            }

            // Place the ship and check for valid placement
            if (placeShipForPlayer(grid1, ship, x, y, orientation) == 0) {
                printf("Ship placed successfully.\n");
                printGrid(grid1);
            } else {
                printf("Invalid placement. Try again.\n");
                i--;  // Repeat this step
            }
        }
    }else{ 
        printf("\n%s, it is your turn to place your ships on the grid: \n", playerName1);
        printf("Enter ship type (C for Carrier, B for Battleship, c for Cruiser, s for Submarine, d for Destroyer), coordinates (row and column), and orientation (0 for horizontal, 1 for vertical):\n");

        for (int i = 0; i < 5; i++) {  // 5 ships per player
            printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
            scanf(" %c %d %d %d", &shipType, &x, &y, &orientation);
            x--;  // Convert to 0-based index
            y--;

            // Determine which ship to place
            switch (shipType) {
                case 'C': ship = &fleet1.carrier; break;
                case 'B': ship = &fleet1.battleship; break;
                case 'c': ship = &fleet1.cruiser; break;
                case 's': ship = &fleet1.submarine; break;
                case 'd': ship = &fleet1.destroyer; break;
                default: 
                    printf("Invalid ship type. Try again.\n");
                    i--;  // Repeat this step
                    continue;
            }

            // Place the ship and check for valid placement
            if (placeShipForPlayer(grid1, ship, x, y, orientation) == 0) {
                printf("Ship placed successfully.\n");
                printGrid(grid1);
            } else {
                printf("Invalid placement. Try again.\n");
                i--;  // Repeat this step
            }
        }

        printf("\n%s, it is now your turn to place your ships on the grid: \n", playerName2);
        printf("Enter ship type (C for Carrier, B for Battleship, c for Cruiser, s for Submarine, d for Destroyer), coordinates (row and column), and orientation (0 for horizontal, 1 for vertical):\n");

        for (int i = 0; i < 5; i++) {  // 5 ships per player
            printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
            scanf(" %c %d %d %d", &shipType, &x, &y, &orientation);
            x--;  // Convert to 0-based index
            y--;

            // Determine which ship to place
            switch (shipType) {
                case 'C': ship = &fleet2.carrier; break;
                case 'B': ship = &fleet2.battleship; break;
                case 'c': ship = &fleet2.cruiser; break;
                case 's': ship = &fleet2.submarine; break;
                case 'd': ship = &fleet2.destroyer; break;
                default: 
                    printf("Invalid ship type. Try again.\n");
                    i--;  // Repeat this step
                    continue;
            }

            // Place the ship and check for valid placement
            if (placeShipForPlayer(grid2, ship, x, y, orientation) == 0) {
                printf("Ship placed successfully.\n");
                printGrid(grid2);
            } else {
                printf("Invalid placement. Try again.\n");
                i--;  // Repeat this step
            }
        }
    }

    char AIdifficulty;
    if (gameMode == '1') {
        printf("Enter Player 2's name: ");
        getchar();  // To clear the buffer from previous input
        fgets(playerName2, sizeof(playerName2), stdin);
        playerName2[strcspn(playerName2, "\n")] = '\0';  // Remove newline
    }else{

        do {
            printf("Select AI difficulty: (E)asy, (M)edium, or (H)ard: ");
            scanf(" %c", &AIdifficulty);
            AIdifficulty = toupper(AIdifficulty); 
        } while (AIdifficulty != 'E' && AIdifficulty != 'M' && AIdifficulty != 'H');  
    }


    
    // initializing hard and easy difficulty
    char difficulty;
    do {
        printf("Select Tracking difficulty: (E)asy or (H)ard: ");
        scanf(" %c", &difficulty);
        difficulty = toupper(difficulty); 
    } while (difficulty != 'E' && difficulty != 'H'); 
        getchar();
    // randomizing who stats
    srand(time(NULL));
    int turn = rand() % 2 + 1;
    int shipsDestroyed;
    while (fleet1.shipsDestroyed != 5 && fleet2.shipsDestroyed != 5) {
        char input[32] = {0};
        char command[16] = {0};
        char target[4] = {0};
        int validCommand = 0;

        int shipsDestroyed;
        Fleet *opponentFleet = (turn == 1) ? &fleet2 : &fleet1;
        char (*oppositeGrid)[GRID_SIZE] = (turn == 1) ? grid2 : grid1;
        char (*playerGrid)[GRID_SIZE] = (turn == 1) ? grid1 : grid2;
        char (*opponentVisibleGrid)[GRID_SIZE] = (turn == 1) ? grid2for1 : grid1for2;
        SmokeScreen *activeSmokes = (turn == 1) ? smokes2 : smokes1;
        int *activeSmokeCount = (turn == 1) ? &activeSmokeCount2 : &activeSmokeCount1;
        int *radarUses = (turn == 1) ? &radarUses1 : &radarUses2;
        int (*heatmap)[GRID_SIZE] = (turn == 1) ? heatmap1 : heatmap2;
        aiTargetHard *aiTargetHard = (turn == 1) ? &aiTargetHard1 : &aiTargetHard2;

        //displayGrids(turn, playerGrid, opponentGrid, player, grid2for1);
        displayGrids(turn, grid1, grid2, grid1for2, grid2for1);

        if ( gameMode == 'A' && turn == 2) {
            if(AIdifficulty == 'E'){
                puts("\nEasy-AI's turn to play");
                aiTurnEasy(oppositeGrid, opponentFleet, turn == 1 ? &AITargetEasy2 : &AITargetEasy1);
            }
            if(AIdifficulty == 'H'){
                puts("\nHard-AI's turn to play");
                aiTurnHard(oppositeGrid, opponentFleet, aiTargetHard, heatmap2, shipSizes, numShips, radarUses);
            }
            UpdateSmokeScreens(activeSmokes, activeSmokeCount);
            UpdateOppositeGrid(grid1,grid1for2,difficulty);
            turn = 1;
            continue;
        }

        puts("Your turn to Play!\n");
        while (!validCommand) {
            if (turn == 1) {
                printf("%s, enter your command (e.g., Fire A5, Radar A5, Smoke A5): ", playerName1);
            } else {
                printf("%s, enter your command (e.g., Fire A5, Radar A5, Smoke A5): ", playerName2);
            }
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0';

            if (sscanf(input, "%15s %3s", command, target) < 2) {
                printf("Invalid input format! Try again.\n");
                continue;
            }

            if (strcmp(command, "Fire") == 0 || strcmp(command, "Artillery") == 0) {
            if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0])) {
                char col = toupper(target[0]);
                int row;
                if (strlen(target) == 2 && isdigit(target[1])) {
                    // Handle single-digit row numbers (e.g., "A5")
                    row = target[1] - '0';
                } else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2])) {
                    // Handle two-digit row numbers (e.g., "A10")
                    row = (target[1] - '0') * 10 + (target[2] - '0');
                } else {
                    printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                    continue;
                }

                if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                    validCommand = 1;
                    if (strcmp(command, "Fire") == 0) {
                        
                        Fire(oppositeGrid, opponentFleet, row - 1, col - 'A', &shipsDestroyed);
                    } else if (opponentFleet->artilleryUnlocked) {
                        Artillery(oppositeGrid, opponentFleet, row - 1, col - 'A');
                    } else {
                        printf("Artillery is not unlocked yet!\n");
                    }
                } else {
                    printf("Invalid position! Use A1-J10.\n");
                }
            } else {
                printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
            }
        }
            else if (strcmp(command, "Torpedo") == 0) {
                if (isalpha(target[0]) && strlen(target) == 1) {
                    char col = toupper(target[0]);
                    if (col >= 'A' && col <= 'J') {
                    validCommand = 1;
                    Torpedo(oppositeGrid, opponentFleet, col - 'A', 'C'); // 'C' for column
            } else {
                    printf("Invalid column! Use A-J for columns.\n");
            }
            } else if (isdigit(target[0])) {
                int row = atoi(target);
                if (row >= 1 && row <= 10) {
                    validCommand = 1;
                    Torpedo(oppositeGrid, opponentFleet, row - 1, 'R'); // 'R' for row
            } else {
            printf("Invalid row! Use 1-10 for rows.\n");
        }
    } else {
        printf("Invalid target for Torpedo! Use a column (A-J) or row (1-10).\n");
    }
}
            else if (strcmp(command, "Radar") == 0) {
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]) && isdigit(target[1])) {
                    char col = toupper(target[0]);
                    int row;
                    if (strlen(target) == 2 && isdigit(target[1])) {
                        // Handle single-digit row numbers (e.g., "A5")
                        row = target[1] - '0';
                    } else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2])) {
                        // Handle two-digit row numbers (e.g., "A10")
                        row = (target[1] - '0') * 10 + (target[2] - '0');
                    } else {
                        printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                        continue;
                    }

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        int enemyFound;
                        RadarSweep(oppositeGrid, 0, row - 1, col - 'A', radarUses, activeSmokes,
                                   *activeSmokeCount, &enemyFound);
                        validCommand = 1; // End turn after using Radar
                        printf("Radar sweep performed. Turn ends.\n");
                    } else {
                        printf("Invalid position! Use A1-J10.\n");
                    }
                } else {
                    printf("Invalid target! Use a format like 'A5'.\n");
                }
            } else if (strcmp(command, "Smoke") == 0) {
                if ((strlen(target) == 2 || strlen(target) == 3) && isalpha(target[0]) && isdigit(target[1])) {
                    char col = toupper(target[0]);
                    int row;
                    if (strlen(target) == 2 && isdigit(target[1])) {
                        // Handle single-digit row numbers (e.g., "A5")
                        row = target[1] - '0';
                    } else if (strlen(target) == 3 && isdigit(target[1]) && isdigit(target[2])) {
                        // Handle two-digit row numbers (e.g., "A10")
                        row = (target[1] - '0') * 10 + (target[2] - '0');
                    } else {
                        printf("Invalid target! Use a format like 'A5' or 'A10'.\n");
                        continue;
                    }

                    if (col >= 'A' && col <= 'J' && row >= 1 && row <= 10) {
                        validCommand = 1;
                        if(turn == 1){
                            DeploySmokeScreen(activeSmokes, activeSmokeCount, row - 1, col - 'A', &fleet2);
                        }else{
                            DeploySmokeScreen(activeSmokes, activeSmokeCount, row - 1, col - 'A', &fleet1);
                        }
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
        UpdateOppositeGrid(grid2,grid2for1,difficulty);
        
        turn = (turn == 1) ? 2 : 1;
    }

    if (fleet1.shipsDestroyed == 5) {
        printf("Player 2 wins!\n");
    } else {
        printf("Player 1 wins!\n");
    }

    return 0;
}

#pragma region HardAI
// Initializes AI target
void initaiTargetHard(aiTargetHard *aiTargetHard) {
    aiTargetHard->isHit = 0;
    aiTargetHard->direction = -1;
    aiTargetHard->radarUsed = 0;
    aiTargetHard->referenceX = -1;
    aiTargetHard->referenceY = -1;
    aiTargetHard->caseNum = 0;
    aiTargetHard->x = -1;
    aiTargetHard->y = -1;
    aiTargetHard->directionIndex = 0;
}

// Heatmap initialization
void initHeatmap(int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            heatmap[i][j] = 0;
        }
    }
    for (int index = 0; index < numShips; index++) {
        int shipSize = shipSizes[index];
        for (int row = 0; row < GRID_SIZE; row++) {
            for (int col = 0; col <= GRID_SIZE - shipSize; col++) {
                for (int pos = 0; pos < shipSize; pos++) {
                    heatmap[row][col + pos]++;
                }
            }
        }
        for (int col = 0; col < GRID_SIZE; col++) {
            for (int row = 0; row <= GRID_SIZE - shipSize; row++) {
                for (int pos = 0; pos < shipSize; pos++) {
                    heatmap[row + pos][col]++;
                }
            }
        }
    }
}

// Heatmap update on hit
void updateHeatmapOnHit(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                        int numShips, int shipDestroyed, char opponentGrid[GRID_SIZE][GRID_SIZE]) {
    // Initially mark the hit cell to prevent re-targeting
    heatmap[x][y] = -999;

    if (shipDestroyed) {
        // Reset heatmap values around the destroyed ship
        for (int dx = -3; dx <= 3; dx++) {
            for (int dy = -3; dy <= 3; dy++) {
                int newX = x + dx;
                int newY = y + dy;
                if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) {
                    heatmap[newX][newY] = 0; // Completely reset the area around the destroyed ship
                }
            }
        }
    } else {
        // Increase probability around the hit
        int directions[4][2] = {{0,1},{1,0},{0,-1},{-1,0}};
        for (int i = 0; i < 4; i++) {
            int newX = x + directions[i][0];
            int newY = y + directions[i][1];
            if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE && heatmap[newX][newY] >= 0) {
                heatmap[newX][newY] += 5;
            }
        }
    }
}

// Heatmap update on miss
void updateHeatmapOnMiss(int heatmap[GRID_SIZE][GRID_SIZE], int x, int y, int shipSizes[],
                         int numShips) {
    heatmap[x][y] = -999; // Mark as fired
}

// AI turn logic for hard difficulty
void aiTurnHard(char grid[GRID_SIZE][GRID_SIZE], Fleet *fleet, aiTargetHard *aiTargetHard,
    int heatmap[GRID_SIZE][GRID_SIZE], int shipSizes[], int numShips, int *radarUses) {
    int x = -1, y = -1;
    int shipDestroyed = 0;

    if (aiTargetHard->isHit) {
        if (!aiTargetHard->radarUsed && *radarUses < MAX_RADAR_USES) {
            int enemyFound = 0;
            RadarSweep(grid, 0, aiTargetHard->x, aiTargetHard->y, radarUses, NULL, 0, &enemyFound);
            printf("AI uses Radar Sweep at %c%d.\n", 'A' + aiTargetHard->y, aiTargetHard->x + 1);
            aiTargetHard->radarUsed = 1; // Mark that Radar has been used for this hit

            if (enemyFound) {
                // Use radar sweep information to determine case
                int southHit = 0, eastHit = 0;
                if (aiTargetHard->x + 1 < GRID_SIZE && grid[aiTargetHard->x + 1][aiTargetHard->y] != '~' &&
                    grid[aiTargetHard->x + 1][aiTargetHard->y] != 'h' && grid[aiTargetHard->x + 1][aiTargetHard->y] != 'm') {
                    southHit = 1;
                }
                if (aiTargetHard->y + 1 < GRID_SIZE && grid[aiTargetHard->x][aiTargetHard->y + 1] != '~' &&
                    grid[aiTargetHard->x][aiTargetHard->y + 1] != 'h' && grid[aiTargetHard->x][aiTargetHard->y + 1] != 'm') {
                    eastHit = 1;
                }

                aiTargetHard->referenceX = aiTargetHard->x;
                aiTargetHard->referenceY = aiTargetHard->y;

                if (southHit) {
                    aiTargetHard->caseNum = 1;
                    aiTargetHard->direction = 1; // South
                } else if (eastHit) {
                    aiTargetHard->caseNum = 2;
                    aiTargetHard->direction = 2; // East
                } else {
                    aiTargetHard->caseNum = 3;
                    aiTargetHard->direction = -1; // Will be set when we enter case 3
                    aiTargetHard->directionIndex = 0; // Start from the first direction
                }
            } else {
                // Radar did not detect ships; proceed with case 3 strategy in subsequent turns
                printf("AI did not detect any ships with radar. Will proceed with case 3 strategy.\n");

                aiTargetHard->caseNum = 3;
                aiTargetHard->direction = -1;
                aiTargetHard->directionIndex = 0;

                // Do not fire immediately; end the turn
                return; // End turn after using radar
            }
            return; // End turn after using radar
        }

        // Proceed with targeting logic
        if (aiTargetHard->caseNum == 1) {
            // Vertical targeting (South then North)
            x = aiTargetHard->referenceX + (aiTargetHard->direction == 1 ? 1 : -1);
            y = aiTargetHard->referenceY;

            if (x >= 0 && x < GRID_SIZE && grid[x][y] != 'm' && grid[x][y] != 'h') {
                // Use Artillery if available
                if (fleet->artilleryUnlocked && x < GRID_SIZE - 1 && y < GRID_SIZE - 1) {
                    Artillery(grid, fleet, x, y);
                    printf("AI uses Artillery at %c%d.\n", 'A' + y, x + 1);
                } else {
                    Fire(grid, fleet, x, y, &shipDestroyed);
                    printf("AI fires at %c%d.\n", 'A' + y, x + 1);
                }

                if (grid[x][y] == 'h') {
                    aiTargetHard->referenceX = x; // Update reference point
                    updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);

                    if (shipDestroyed) {
                        // Ship destroyed, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                } else {
                    if (aiTargetHard->direction == 1) {
                        // Switch direction to North
                        aiTargetHard->direction = 0;
                        aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit
                    } else {
                        // Both directions tried, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                    updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
                }
            } else {
                if (aiTargetHard->direction == 1) {
                    // Switch direction to North
                    aiTargetHard->direction = 0;
                    aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit
                } else {
                    // Both directions tried, reset AI targeting
                    initaiTargetHard(aiTargetHard);
                }
            }
            return; // End turn after firing once
        } else if (aiTargetHard->caseNum == 2) {
            // Horizontal targeting (East then West)
            x = aiTargetHard->referenceX;
            y = aiTargetHard->referenceY + (aiTargetHard->direction == 2 ? 1 : -1);

            if (y >= 0 && y < GRID_SIZE && grid[x][y] != 'm' && grid[x][y] != 'h') {
                // Use Artillery if available
                if (fleet->artilleryUnlocked && x < GRID_SIZE - 1 && y < GRID_SIZE - 1) {
                    Artillery(grid, fleet, x, y);
                    printf("AI uses Artillery at %c%d.\n", 'A' + y, x + 1);
                } else {
                    Fire(grid, fleet, x, y, &shipDestroyed);
                    printf("AI fires at %c%d.\n", 'A' + y, x + 1);
                }

                if (grid[x][y] == 'h') {
                    aiTargetHard->referenceY = y; // Update reference point
                    updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);

                    if (shipDestroyed) {
                        // Ship destroyed, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                } else {
                    if (aiTargetHard->direction == 2) {
                        // Switch direction to West
                        aiTargetHard->direction = 3;
                        aiTargetHard->referenceY = aiTargetHard->y; // Reset to initial hit
                    } else {
                        // Both directions tried, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    }
                    updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
                }
            } else {
                if (aiTargetHard->direction == 2) {
                    // Switch direction to West
                    aiTargetHard->direction = 3;
                    aiTargetHard->referenceY = aiTargetHard->y; // Reset to initial hit
                } else {
                    // Both directions tried, reset AI targeting
                    initaiTargetHard(aiTargetHard);
                }
            }
            return; // End turn after firing once
        } else if (aiTargetHard->caseNum == 3) {
            // Targeting in all directions
            int directionSequence[4] = {0, 1, 3, 2}; // North, South, West, East

            if (aiTargetHard->directionIndex >= 4) {
                // All directions tried, reset AI targeting
                initaiTargetHard(aiTargetHard);
                return; // End turn
            }

            aiTargetHard->direction = directionSequence[aiTargetHard->directionIndex];
            x = aiTargetHard->referenceX;
            y = aiTargetHard->referenceY;

            // Determine next cell in current direction
            switch (aiTargetHard->direction) {
                case 0: // North
                    x = x - 1;
                    break;
                case 1: // South
                    x = x + 1;
                    break;
                case 2: // East
                    y = y + 1;
                    break;
                case 3: // West
                    y = y - 1;
                    break;
            }

            // Check if coordinates are valid and not already fired upon
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE &&
                grid[x][y] != 'm' && grid[x][y] != 'h') {
                // Use Artillery if available
                if (fleet->artilleryUnlocked && x < GRID_SIZE - 1 && y < GRID_SIZE - 1) {
                    Artillery(grid, fleet, x, y);
                    printf("AI uses Artillery at %c%d.\n", 'A' + y, x + 1);
                } else {
                    Fire(grid, fleet, x, y, &shipDestroyed);
                    printf("AI fires at %c%d.\n", 'A' + y, x + 1);
                }

                if (grid[x][y] == 'h') {
                    // Hit, update reference point and continue in same direction in next turn
                    aiTargetHard->referenceX = x;
                    aiTargetHard->referenceY = y;
                    updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);

                    if (shipDestroyed) {
                        // Ship destroyed, reset AI targeting
                        initaiTargetHard(aiTargetHard);
                    } else {
                        // Determine if the ship is vertical or horizontal based on direction
                        if (aiTargetHard->direction == 0 || aiTargetHard->direction == 1) {
                            aiTargetHard->caseNum = 1; // Vertical ship
                        } else {
                            aiTargetHard->caseNum = 2; // Horizontal ship
                        }
                    }
                } else {
                    // Miss, move to next direction
                    aiTargetHard->directionIndex++;
                    aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit position
                    aiTargetHard->referenceY = aiTargetHard->y;
                    updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
                }
            } else {
                // Invalid coordinate or already fired upon, move to next direction
                aiTargetHard->directionIndex++;
                aiTargetHard->referenceX = aiTargetHard->x; // Reset to initial hit position
                aiTargetHard->referenceY = aiTargetHard->y;
            }
            return; // End AI's turn after one fire
        }
    } else {
        // AI has no current target, use heatmap to find the next target
        int maxHeat = -1;
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                if (heatmap[i][j] > maxHeat && grid[i][j] != 'm' && grid[i][j] != 'h') {
                    maxHeat = heatmap[i][j];
                    x = i;
                    y = j;
                }
            }
        }

        // Use Fire when selecting target from heatmap
        Fire(grid, fleet, x, y, &shipDestroyed);
        printf("AI fires at %c%d.\n", 'A' + y, x + 1);

        if (grid[x][y] == 'h') {
            aiTargetHard->isHit = 1;
            aiTargetHard->x = x;
            aiTargetHard->y = y;
            aiTargetHard->direction = -1;
            aiTargetHard->radarUsed = 0; // Reset radarUsed for new hit
            aiTargetHard->referenceX = x;
            aiTargetHard->referenceY = y;
            aiTargetHard->caseNum = 0;
            updateHeatmapOnHit(heatmap, x, y, shipSizes, numShips, shipDestroyed, grid);
        } else {
            updateHeatmapOnMiss(heatmap, x, y, shipSizes, numShips);
        }
        return; // End AI's turn after firing once
    }
}

#pragma endregion HardAI