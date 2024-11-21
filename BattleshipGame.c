#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#define GRID_SIZE 10

// keeps track of placed ships on grid1
// must be reset for player 2
char placedShips [6] = "NNNNN";

// Ship structure to define each ship type
typedef struct {
    char type;
    int size;
    int hits;  // Tracks hits for each ship
} Ship;

// Fleet structure to define a collection of ships for each player
typedef struct {
    Ship carrier;
    Ship battleship;
    Ship destroyer;
    Ship submarine;
    int shipsDestroyed;
    int hasArtillery;
    int hasTorpedo;
} Fleet;

// Smoke Screen structure to define a smoke screen effect with coordinates and duration
typedef struct {
    int x, y;           // Top-left coordinate of the 2x2 smoke-covered area
    int turnsRemaining; // Number of turns left for the smoke screen to be active
} SmokeScreen;


typedef enum { MISS, HIT, SUNK, INVALID } FireResult;
typedef enum { ARTILLERY_MISS, ARTILLERY_HIT, ARTILLERY_SUNK, ARTILLERY_INVALID } ArtilleryResult;
typedef enum { TORPEDO_MISS, TORPEDO_HIT, TORPEDO_SUNK, TORPEDO_INVALID } TorpedoResult;
typedef enum { NO_ENEMY, ENEMY_FOUND, INVALID_RADAR } RadarResult;
typedef enum { SUCCESS, FAILURE, INVALID_COORDINATES, MAX_SMOKE_REACHED } SmokeScreenResult;

// Function prototypes
void initGrid(char grid[GRID_SIZE][GRID_SIZE]);
void printGrid(char grid[GRID_SIZE][GRID_SIZE]);
void initializeFleet(Fleet *fleet);
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation);
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]);
int isValidCommand(const char *command, char pos, int row, char grid[GRID_SIZE][GRID_SIZE]);
FireResult Fire(char opponentGrid[GRID_SIZE][GRID_SIZE], char viewGrid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, Fleet *currentFleet, int x, int y);
ArtilleryResult Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, int x, int y);
TorpedoResult Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, char rowOrCol);
Ship* getShipByType(Fleet *fleet, char shipType);
const char* getShipNameByType(char type);
RadarResult RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses, SmokeScreen smokes[], int activeSmokeCount);
SmokeScreenResult DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y);
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount);
Ship* getShipByType(Fleet *fleet, char shipType);
void clearScreen();

char currentPlayerName[20], opponentPlayerName[20];

int main() {
    char grid1[GRID_SIZE][GRID_SIZE];
    char grid2[GRID_SIZE][GRID_SIZE];

    // These're displayed at opponent's turn
    char grid1for2[GRID_SIZE][GRID_SIZE];
    char grid2for1[GRID_SIZE][GRID_SIZE];
    
    Fleet fleet1, fleet2;
    initializeFleet(&fleet1);
    initializeFleet(&fleet2);

    initGrid(grid1);
    initGrid(grid2);
    printGrid(grid1);

    initGrid(grid1for2);
    initGrid(grid2for1);

    int x, y, orientation;
    char shipType;
    Ship *ship;
    
    printf("Enter ship type (C for Carrier, B for Battleship, D for Destroyer, S for Submarine), coordinates (row and column), and orientation (0 for horizontal, 1 for vertical):\n");


    for (int i = 0; i < 4; i++) {  // 4 ships per player
        printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
        scanf(" %c %d %d %d", &shipType, &x, &y, &orientation);
        x--;  // Convert to 0-based index
        y--;

        // Determine which ship to place
        switch (shipType) {
            case 'C': ship = &fleet1.carrier; break;
            case 'B': ship = &fleet1.battleship; break;
            case 'D': ship = &fleet1.destroyer; break;
            case 'S': ship = &fleet1.submarine; break;
            default:
            printf("Invalid ship type. Try again.\n");
            i--;  // Repeat this step
            continue;
            
        }

        // Place the ship and check for valid placement
        if (placeShip(grid1, ship, x, y, orientation) == 0) {
            printf("Ship placed successfully.\n");
            printGrid(grid1);
        } else {
            printf("Invalid placement. Try again.\n");
            i--;  // Repeat this step
        }
    }
   for(int i = 0; i<5; i++ ){
        placedShips[i] = 'N';
    }
    printf("Player 2:\n");
    char input[20];
    for (int i = 0; i < 4; i++) {  // 4 ships per player based on your requirements
    printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, " %c %d %d %d", &shipType, &x, &y, &orientation) != 4) {
        printf("Invalid input. Try again.\n");
        i--;
        continue;
    }
    x--;  // Convert to 0-based index
    y--;

        // Determine which ship to place
        switch (shipType) {
            case 'C': ship = &fleet2.carrier; break;
            case 'B': ship = &fleet2.battleship; break;
            case 'S': ship = &fleet2.submarine; break;
            case 'D': ship = &fleet2.destroyer; break;
            default: 
                printf("Invalid ship type. Try again.\n");
                i--;  // Repeat this step
                continue;
        }

        // Place the ship and check for valid placement
        if (placeShip(grid2, ship, x, y, orientation) == 0) {
            printf("Ship placed successfully.\n");
            printGrid(grid2);
        } else {
            printf("Invalid placement. Try again.\n");
            i--;  // Repeat this step
        }
    }
    
    // Now, it is time to start the game
    // Program decides randomly whom to go first
    srand(time(NULL));
    
    int turn = rand() % 2 + 1;
    char command[10];
    char pos;
    int row, col;
    Fleet *currentFleet, *opposingFleet;
    int radarUses1 = 0, radarUses2 = 0;
    int smokeScreens1 = 0, smokeScreens2 = 0;
    int *radarUses = (turn == 1) ? &radarUses1 : &radarUses2;
    int *smokeScreens = (turn == 1) ? &smokeScreens1 : &smokeScreens2; 
    
    while(fleet1.shipsDestroyed != 5 && fleet2.shipsDestroyed != 5){
        char (*grid)[GRID_SIZE];
        char (*oppositeGrid)[GRID_SIZE];
        if (turn == 1) {
            grid = grid1;
            oppositeGrid = grid2;
            currentFleet = &fleet1;
            opposingFleet = &fleet2;
            strcpy(currentPlayerName, "Player 1");
            strcpy(opponentPlayerName, "Player 2");
        } else {
            grid = grid2;
            oppositeGrid = grid1;
            currentFleet = &fleet2;
            opposingFleet = &fleet1;
            strcpy(currentPlayerName, "Player 2");
            strcpy(opponentPlayerName, "Player 1");
        }



        printf("Player %d to move:\n",turn);
        //displayGrids(turn, grid1, grid2, grid1for2, grid2for1);
        char (*oppositeViewGrid)[GRID_SIZE];

    do {
        printf("Enter your command (e.g., Fire A5): ");
        // Read the input
        if (fgets(input, sizeof(input), stdin)) {
            // Trim newline character if present
            input[strcspn(input, "\n")] = 0;
            // Parse the command, column (pos), and row
            if (sscanf(input, "%s %c%d", command, &pos, &row) != 3) {
            printf("Invalid format! Please use the format <Command> <Column><Row> (e.g., Fire A5)\n");
            continue;
            }
            // Convert column to uppercase for consistency
            pos = toupper(pos);
            // Validate the command
           if (!isValidCommand(command, pos, row, oppositeViewGrid)) {
                printf("Invalid command! Try again.\n");
                continue;
            }else {
            break;  // Exit loop if the command is valid
            }
        } else {
        printf("Error reading input. Try again.\n");
        continue;
        }
    } while (1);

// Convert row and column to zero-based index
col = pos - 'A';
row--;  // Convert to 0-based index


//Executing the available commands/moves
if (strcmp(command, "Fire") == 0) {
    FireResult result = Fire(oppositeGrid, oppositeViewGrid, opposingFleet, currentFleet, row, col);
    switch (result) {
        case HIT:
            printf("Great shot, %s! Keep going.\n", currentPlayerName);
            break;
        case MISS:
            printf("Missed! Better luck next time, %s.\n", currentPlayerName);
            break;
        case SUNK:
            printf("You sunk one of %s's ships!\n", opponentPlayerName);
            break;
        case INVALID:
            printf("Invalid move. Turn skipped.\n");
            break; // Skip the rest of this turn
}
        } else if (strcmp(command, "Artillery") == 0 && currentFleet->hasArtillery) {
            ArtilleryResult result = Artillery(oppositeGrid, opposingFleet, row, col);
            currentFleet->hasArtillery = 0; // Consume the special move
            printf("Result: %d\n", result);
        } else if (strcmp(command, "Torpedo") == 0 && currentFleet->hasTorpedo) {
            TorpedoResult result = Torpedo(oppositeGrid, opposingFleet, pos);
            currentFleet->hasTorpedo = 0; // Consume the special move
            printf("Result: %d\n", result);
        } else if (strcmp(command, "Smoke") == 0) {
            SmokeScreenResult result = DeploySmokeScreen(NULL, NULL, row, col);
            printf("Smoke Screen Deployment Result: %d\n", result);
        } else if (strcmp(command, "Radar") == 0) {
            int radarUses = 0;
            RadarResult result = RadarSweep(oppositeGrid, 1, row, col, &radarUses, NULL, 0);
            printf("Radar Sweep Result: %d\n", result);
        } else {
            printf("Invalid command or move not unlocked!\n");
            continue;
        }
        // Check for game end
        if (opposingFleet->shipsDestroyed == 4) {
        printf("%s wins! All of %s's ships have been sunk.\n", currentPlayerName, opponentPlayerName);
        break;
        }
        // Switch turns
        turn = (turn == 1) ? 2 : 1;
        // Clear screen for secrecy 
        printf("Press Enter to end your turn...");
        getchar(); 
        clearScreen();
    }

    displayGrids(2, grid1, grid2, grid1for2, grid2for1);
    return 0;
}

FireResult Fire(char opponentGrid[GRID_SIZE][GRID_SIZE], char viewGrid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, Fleet *currentFleet, int x, int y) {
    char shipType = opponentGrid[x][y];  // Retrieve the type of the ship at the targeted cell
    Ship *hitShip = getShipByType(opponentFleet, shipType);

    // Validate coordinates
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        printf("Invalid coordinates! You lose your turn.\n");
        return INVALID;
    }

    // Check if the cell was already targeted in the player's view
    if (viewGrid[x][y] == 'h' || viewGrid[x][y] == 'm') {
        printf("This cell has already been targeted. You lose your turn.\n");
        return INVALID;
    }

    // Handle hit or miss
    if (opponentGrid[x][y] == '~') {
        opponentGrid[x][y] = 'm';  // Mark as miss on opponent's grid
        viewGrid[x][y] = 'm';      // Mark as miss on player's view
        return MISS;
    } else {
        char shipType = opponentGrid[x][y];  // Get ship type
        opponentGrid[x][y] = 'h';  // Mark as hit on opponent's grid
        viewGrid[x][y] = 'h';      // Mark as hit on player's view

        // Update ship status
        Ship *hitShip = getShipByType(opponentFleet, shipType);
        if (hitShip) {
            hitShip->hits++;
            if (hitShip->hits == hitShip->size) {
                printf("You sunk the %s!\n", getShipNameByType(shipType));
                opponentFleet->shipsDestroyed++;
                return SUNK;
            }
        }
        return HIT;
    }
    if (hitShip->hits == hitShip->size) {
    printf("You sunk the %s!\n", getShipNameByType(shipType));
    opponentFleet->shipsDestroyed++;

    // Unlock Artillery for next turn
    currentFleet->hasArtillery = 1;

    // If the player has sunk three ships, unlock Torpedo
    if (opponentFleet->shipsDestroyed == 3) {
        currentFleet->hasTorpedo = 1;
    }

    return SUNK;
    }

}

RadarResult RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses, SmokeScreen smokes[], int activeSmokeCount) {
    // Check if the radar is being used in an area covered by smoke
    for (int i = 0; i < activeSmokeCount; i++) {
        if (x >= smokes[i].x && x < smokes[i].x + 2 && y >= smokes[i].y && y < smokes[i].y + 2) {
            printf("No enemy ships found (due to smoke).\n");
            return NO_ENEMY;  // Radar sweep blocked by smoke
        }
    }
    // Check if radar uses exceed the limit
    if (*radarUses >= 3) {
        printf("No more radar sweeps available. You've used all your attempts.\n");
        return INVALID_RADAR;  // Exceeds radar use limit
    }

    // Validate that the top-left coordinate is within bounds for a 2x2 area
    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        printf("Invalid coordinates for radar sweep. Please try again.\n");
        return INVALID_RADAR;  // Area exceeds grid bounds
    }

    // Perform the radar sweep over the 2x2 area
    int enemyFound = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            char cell = grid[x + i][y + j];
            if (cell != '~' && cell != 'h' && cell != 'm') {
                enemyFound = 1;  // A ship is present in the area
            }
        }
    }

    // Increment radar uses for the player
    (*radarUses)++;

    // Output results based on difficulty and presence of ships
    if (enemyFound) {
        printf("Enemy ships found.\n");
        return ENEMY_FOUND;
    } else {
        printf("No enemy ships found.\n");
        return NO_ENEMY;
    }
}

SmokeScreenResult DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y) {
    // Check if the player has available smoke screens to deploy
    if (*activeSmokeCount >= 2) {
        printf("Maximum smoke screens deployed. Cannot deploy more.\n");
        return MAX_SMOKE_REACHED;
    }

    // Validate that the 2x2 area is within bounds
    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        printf("Invalid coordinates for smoke screen deployment. Please try again.\n");
        return INVALID_COORDINATES;
    }

    // Deploy smoke screen
    smokes[*activeSmokeCount].x = x;
    smokes[*activeSmokeCount].y = y;
    smokes[*activeSmokeCount].turnsRemaining = 3; // Set duration to 3 turns
    (*activeSmokeCount)++;

    printf("Smoke screen deployed at %c%d.\n", 'A' + y, x + 1);
    return SUCCESS;
}

void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount) {
    for (int i = 0; i < *activeSmokeCount; i++) {
        smokes[i].turnsRemaining--;
        if (smokes[i].turnsRemaining <= 0) {
            // Remove expired smoke screen
            printf("Smoke screen at %c%d has expired.\n", 'A' + smokes[i].y, smokes[i].x + 1);
            // Shift remaining smoke screens to fill the gap
            for (int j = i; j < *activeSmokeCount - 1; j++) {
                smokes[j] = smokes[j + 1];
            }
            (*activeSmokeCount)--;
            i--; // Adjust index due to removal
        }
    }
}

// Helper function to find the ship by type
Ship* getShipByType(Fleet *fleet, char shipType) {
    switch (shipType) {
        case 'C': return &fleet->carrier;
        case 'B': return &fleet->battleship;
        case 'D': return &fleet->destroyer;
        case 'S': return &fleet->submarine;
        default: return NULL;
    }
}

const char* getShipNameByType(char type) {
    switch (type) {
        case 'C': return "Carrier";
        case 'B': return "Battleship";
        case 'D': return "Destroyer";
        case 'S': return "Submarine";
        default: return "Unknown Ship";
    }
}

ArtilleryResult Artillery(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, int x, int y) {
    // Check if the given coordinate of the 2x2 is within grid boundaries
    if (x < 0 || x >= GRID_SIZE - 1 || y < 0 || y >= GRID_SIZE - 1) {
        return ARTILLERY_INVALID;
    }
    int hitCount = 0;  // Tracks the number of successful hits
    // Looping through the 2x2
    for (int i = x; i <= x + 1; i++) {
        for (int j = y; j <= y + 1; j++) {
            if (grid[i][j] == '~') {
                grid[i][j] = 'm';  // The cell is an empty water; thus, is marked as miss
            } else if (grid[i][j] != 'h' && grid[i][j] != 'm') {
                grid[i][j] = 'h';  // The cell is part of a ship; thus, is marked as hit
                hitCount++; 
                // Retrieve ship's type and relate it back to opponent's fleet
                Ship *hitShip = getShipByType(opponentFleet, grid[i][j]);
                // Checks if the ship has been sunk
                if (hitShip) {
                    hitShip->hits++;
                    if (hitShip->hits == hitShip->size) {
                        printf("%s has been sunk!\n", getShipNameByType(grid[i][j]));
                        opponentFleet->shipsDestroyed++; // Update fleet's destroyed ship count
                    }
                }
            }
        }
    }
    return hitCount > 0 ? ARTILLERY_HIT : ARTILLERY_MISS;
}

TorpedoResult Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, char rowOrCol) {
    // Determines if the user wants to target a row or column based on inpute
    int index = (rowOrCol >= 'A' && rowOrCol <= 'J') ? rowOrCol - 'A' : rowOrCol - '1';
    // Checks that the index is within grid boundaries
    if (index < 0 || index >= GRID_SIZE) {
        return TORPEDO_INVALID;
    }
    int hitCount = 0;
    // Loop over target row or column based on index
    for (int i = 0; i < GRID_SIZE; i++) {
        // For column targets:
        if (rowOrCol >= 'A' && rowOrCol <= 'J') {
            if (grid[i][index] == '~') {
                grid[i][index] = 'm';
            } else if (grid[i][index] != 'h' && grid[i][index] != 'm') {
                grid[i][index] = 'h';
                hitCount++;
                Ship *hitShip = getShipByType(opponentFleet, grid[i][index]);
                if (hitShip) {
                    hitShip->hits++;
                    if (hitShip->hits == hitShip->size) {
                        printf("%s has been sunk!\n", getShipNameByType(grid[i][index]));
                        opponentFleet->shipsDestroyed++;
                    }
                }
            }
        } else {
            // For row targets:
            if (grid[index][i] == '~') {
                grid[index][i] = 'm';
            } else if (grid[index][i] != 'h' && grid[index][i] != 'm') {
                grid[index][i] = 'h';
                hitCount++;
                Ship *hitShip = getShipByType(opponentFleet, grid[index][i]);
                if (hitShip) {
                    hitShip->hits++; 
                    if (hitShip->hits == hitShip->size) {
                        printf("%s has been sunk!\n", getShipNameByType(grid[index][i]));
                        opponentFleet->shipsDestroyed++;  // Update fleet's destroyed ship
                    }
                }
            }
        }
    }

    return hitCount > 0 ? TORPEDO_HIT : TORPEDO_MISS;
}

TorpedoResult Torpedo(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, char rowOrCol) {
    // Determines if the user wants to target a row or column based on inpute
    int index = (rowOrCol >= 'A' && rowOrCol <= 'J') ? rowOrCol - 'A' : rowOrCol - '1';
    // Checks that the index is within grid boundaries
    if (index < 0 || index >= GRID_SIZE) {
        return TORPEDO_INVALID;
    }
    int hitCount = 0;
    // Loop over target row or column based on index
    for (int i = 0; i < GRID_SIZE; i++) {
        // For column targets:
        if (rowOrCol >= 'A' && rowOrCol <= 'J') {
            if (grid[i][index] == '~') {
                grid[i][index] = 'm';
            } else if (grid[i][index] != 'h' && grid[i][index] != 'm') {
                grid[i][index] = 'h';
                hitCount++;
                Ship *hitShip = getShipByType(opponentFleet, grid[i][index]);
                if (hitShip) {
                    hitShip->hits++;
                    if (hitShip->hits == hitShip->size) {
                        printf("%s has been sunk!\n", getShipNameByType(grid[i][index]));
                        opponentFleet->shipsDestroyed++;
                    }
                }
            }
        } else {
            // For row targets:
            if (grid[index][i] == '~') {
                grid[index][i] = 'm';
            } else if (grid[index][i] != 'h' && grid[index][i] != 'm') {
                grid[index][i] = 'h';
                hitCount++;
                Ship *hitShip = getShipByType(opponentFleet, grid[index][i]);
                if (hitShip) {
                    hitShip->hits++; 
                    if (hitShip->hits == hitShip->size) {
                        printf("%s has been sunk!\n", getShipNameByType(grid[index][i]));
                        opponentFleet->shipsDestroyed++;  // Update fleet's destroyed ship
                    }
                }
            }
        }
    }

    return hitCount > 0 ? TORPEDO_HIT : TORPEDO_MISS;
}

// used for checking if a command is right
int isValidCommand(const char *command, char pos, int row, char grid[GRID_SIZE][GRID_SIZE]) {
    // Ensure the command is one of the recognized commands
    if (!(strcmp(command, "Fire") == 0 ||
          strcmp(command, "Smoke") == 0 ||
          strcmp(command, "Radar") == 0 ||
          strcmp(command, "Artillery") == 0 ||
          strcmp(command, "Torpedo") == 0)) {
        return 0; // Invalid command
    }

    // Check if the coordinates are within grid bounds
    if (pos < 'A' || pos > 'J' || row < 1 || row > GRID_SIZE) {
        return 0; // Out of bounds
    }

    // Convert row and column to zero-based indices
    int col = pos - 'A';
    int x = row - 1;

    // Use player's view of the opponent's grid
    if (grid[x][col] == 'h' || grid[x][col] == 'm') {
        return 0; // Cell already targeted
    }

    return 1; // Valid command
}

// Initialize each fleet with its ships and sizes
void initializeFleet(Fleet *fleet) {
    fleet->carrier = (Ship){'C', 5, 0};
    fleet->battleship = (Ship){'B', 4, 0};
    fleet->submarine = (Ship){'S', 3, 0};
    fleet->destroyer = (Ship){'D', 2, 0};
    fleet->shipsDestroyed = 0;
    fleet->hasArtillery = 0;
    fleet->hasTorpedo = 0;
}

// Initialize the grid with '~' for water
void initGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '~';
        }
    }
}

// Print the grid with column and row headers
void printGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    printf("   A B C D E F G H I J\n");  // Column headers
    for (int i = 0; i < GRID_SIZE; i++) {
        printf("%2d ", i + 1);  // Row numbers
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
}


// returns 1 if ship's already placed on the grid
// or 0 if it hasn't yet been placed
// and then adds it to the placedShips array
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
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation) {
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


// A player in the easy mode should
// be able to see his own grid
// and his opponent's grid
// with prev Ms and Hs

void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]){
    if(turn == 1){
        printf("Your own grid:\n");
        printGrid(grid1);
        printf("Your view of opponent's grid:\n");
        printGrid(grid2for1);
    }else{
        printf("Your own grid:\n");
        printGrid(grid2);
        printf("Your view of opponent's grid:\n");
        printGrid(grid1for2);
    }
}
void clearScreen() {
    #ifdef _WIN32
    system("cls");  // Windows system command to clear the screen
    #else
    system("clear");  // Unix-based system command to clear the screen
    #endif
}
