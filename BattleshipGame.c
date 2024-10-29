#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

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
    Ship cruiser;
    Ship submarine;
    Ship destroyer;
    int shipsDestroyed;
} Fleet;

// Smoke Screen structure to define a smoke screen effect with coordinates and duration
typedef struct {
    int x, y;           // Top-left coordinate of the 2x2 smoke-covered area
    int turnsRemaining; // Number of turns left for the smoke screen to be active
} SmokeScreen;


typedef enum { MISS, HIT, SUNK, INVALID } FireResult;
typedef enum { NO_ENEMY, ENEMY_FOUND, INVALID_RADAR } RadarResult;
typedef enum { SUCCESS, FAILURE, INVALID_COORDINATES, MAX_SMOKE_REACHED } SmokeScreenResult;

// Function prototypes
void initGrid(char grid[GRID_SIZE][GRID_SIZE]);
void printGrid(char grid[GRID_SIZE][GRID_SIZE]);
void initializeFleet(Fleet *fleet);
int placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship, int x, int y, int orientation);
void displayGrids(int turn, char grid1[GRID_SIZE][GRID_SIZE], char grid2[GRID_SIZE][GRID_SIZE], char grid1for2[GRID_SIZE][GRID_SIZE], char grid2for1[GRID_SIZE][GRID_SIZE]);
int isValidCommand(const char *command, char pos, int row,char grid[GRID_SIZE][GRID_SIZE]);
FireResult Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, int difficulty, int x, int y, SmokeScreen smokes[], int activeSmokeCount);
Ship* getShipByType(Fleet *fleet, char shipType);
const char* getShipNameByType(char type);
RadarResult RadarSweep(char grid[GRID_SIZE][GRID_SIZE], int difficulty, int x, int y, int *radarUses, SmokeScreen smokes[], int activeSmokeCount);
SmokeScreenResult DeploySmokeScreen(SmokeScreen smokes[], int *activeSmokeCount, int x, int y);
void UpdateSmokeScreens(SmokeScreen smokes[], int *activeSmokeCount);


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
    
    printf("Enter ship type (C for Carrier, B for Battleship, c for Cruiser, s for Submarine, d for Destroyer), coordinates (row and column), and orientation (0 for horizontal, 1 for vertical):\n");


    /* This is working but mothballed for testing purpoeses */
    // for (int i = 0; i < 5; i++) {  // 5 ships per player
    //     printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
    //     scanf(" %c %d %d %d", &shipType, &x, &y, &orientation);
    //     x--;  // Convert to 0-based index
    //     y--;

    //     // Determine which ship to place
    //     switch (shipType) {
    //         case 'C': ship = &fleet1.carrier; break;
    //         case 'B': ship = &fleet1.battleship; break;
    //         case 'c': ship = &fleet1.cruiser; break;
    //         case 's': ship = &fleet1.submarine; break;
    //         case 'd': ship = &fleet1.destroyer; break;
    //         default: 
    //             printf("Invalid ship type. Try again.\n");
    //             i--;  // Repeat this step
    //             continue;
    //     }

    //     // Place the ship and check for valid placement
    //     if (placeShip(grid1, ship, x, y, orientation) == 0) {
    //         printf("Ship placed successfully.\n");
    //         printGrid(grid1);
    //     } else {
    //         printf("Invalid placement. Try again.\n");
    //         i--;  // Repeat this step
    //     }
    // }
//    for(int i = 0; i<5; i++ ){
//         placedShips[i] = 'N';
//     }
    // printf("Player 2:\n");
    // for (int i = 0; i < 5; i++) {  // 5 ships per player
    //     printf("Place a ship (e.g., C 3 4 0 for Carrier at row 3, column 4, horizontal): ");
    //     scanf(" %c %d %d %d", &shipType, &x, &y, &orientation);
    //     x--;  // Convert to 0-based index
    //     y--;

    //     // Determine which ship to place
    //     switch (shipType) {
    //         case 'C': ship = &fleet2.carrier; break;
    //         case 'B': ship = &fleet2.battleship; break;
    //         case 'c': ship = &fleet2.cruiser; break;
    //         case 's': ship = &fleet2.submarine; break;
    //         case 'd': ship = &fleet2.destroyer; break;
    //         default: 
    //             printf("Invalid ship type. Try again.\n");
    //             i--;  // Repeat this step
    //             continue;
    //     }

    //     // Place the ship and check for valid placement
    //     if (placeShip(grid2, ship, x, y, orientation) == 0) {
    //         printf("Ship placed successfully.\n");
    //         printGrid(grid2);
    //     } else {
    //         printf("Invalid placement. Try again.\n");
    //         i--;  // Repeat this step
    //     }
    //}

    placeShip(grid1, &fleet1.carrier, 0, 0, 0);     // Horizontal placement at top row
    placeShip(grid1, &fleet1.battleship, 2, 0, 1);  // Vertical placement at column 0
    placeShip(grid1, &fleet1.cruiser, 4, 4, 0);     // Horizontal in the middle
    placeShip(grid1, &fleet1.submarine, 6, 6, 1);   // Vertical towards bottom
    placeShip(grid1, &fleet1.destroyer, 8, 3, 0);   // Horizontal near bottom
    for(int i = 0; i<5; i++ ){
        placedShips[i] = 'N';
    }
    placeShip(grid2, &fleet2.carrier, 1, 1, 0);     // Opponent's Carrier
    placeShip(grid2, &fleet2.battleship, 3, 1, 1);  // Opponent's Battleship
    placeShip(grid2, &fleet2.cruiser, 5, 5, 0);     // Opponent's Cruiser
    placeShip(grid2, &fleet2.submarine, 7, 7, 1);   // Opponent's Submarine
    placeShip(grid2, &fleet2.destroyer, 9, 4, 0);   // Opponent's Destroyer



    // Now, it is time to start the game
    // Program decides randomly whom to go first
    srand(time(NULL));
    
    int turn = rand() % 2 + 1;
    char* command;
    char pos;
    int col,row; 
    while(fleet1.shipsDestroyed != 5 && fleet2.shipsDestroyed != 5){
        char (*grid)[GRID_SIZE];
        char (*oppositeGrid)[GRID_SIZE];
        if (turn == 1) {
            grid = grid1;
            oppositeGrid = grid2;
        } else {
            grid = grid2;
            oppositeGrid = grid1;
        }



        printf("Player %d to move:\n",turn);
        fleet1.shipsDestroyed++;
        //displayGrids(turn, grid1, grid2, grid1for2, grid2for1);

        do { // Checking if command is correct
            printf("Enter your command (e.g., Fire A5): ");
            scanf("%s %c %d", command, &pos, &row);
            if (!isValidCommand(command, pos, row,oppositeGrid)) {
                printf("Invalid command! Try again!\n");
            }
        } while (!isValidCommand(command, pos, row,oppositeGrid));
        col = pos - 'A';
        printf("%c",&oppositeGrid[row][col]);
        // if( strcmp(command,"Fire")==0 ) {
        //     printf("Firing!\n");
        //     Fire(oppositeGrid,1,row,col);
        // }


        
        
        if(turn==1){
            turn=2;
        }else{turn=1;}
    }

    displayGrids(2, grid1, grid2, grid1for2, grid2for1);

    return 0;
}

// void Fire(char grid[GRID_SIZE][GRID_SIZE],int difficulty, int x,int y){
//     if(difficulty == 1){
//         if(grid[x][y] == '~'){
//             grid[x][y] = 'm';
//             printf("Missed!\n");
//         }else{
//             grid[x][y] = 'h';
//             printf("Hit!\n");
//         }
//     }else{

//     }

// }

FireResult Fire(char grid[GRID_SIZE][GRID_SIZE], Fleet *opponentFleet, int difficulty, int x, int y, SmokeScreen smokes[], int activeSmokeCount) {
    // Validate if coordinates are within grid boundaries
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) {
        return INVALID;  // Invalid coordinates
    }

    // Check if the cell has already been targeted
    if (grid[x][y] == 'h' || grid[x][y] == 'm') {
        if (difficulty == 1) { // Easy mode
            printf("You've already targeted this spot. Try again.");
            return INVALID;  // Prompt the player to try again
            } else if (difficulty == 2) { // Hard mode
            printf("You've already targeted this spot. You lose your turn.");
            return MISS;  // Player loses the turn
        }
    }

    // Determine if the target is a hit or miss
    if (grid[x][y] != '~' && grid[x][y] != 'h' && grid[x][y] != 'm') {
        // It's a hit
        printf("Hit!");
        char shipType = grid[x][y];
        grid[x][y] = 'h';  // Mark the cell as hit

        // Update the ship's hits count in the opponent's fleet
        Ship *hitShip = getShipByType(opponentFleet, shipType);
        if (hitShip == NULL) return INVALID;  // In case of an unexpected value

        hitShip->hits++;

        // Check if the ship is sunk
        if (hitShip->hits == hitShip->size) {
            printf("%s has been sunk!", getShipNameByType(shipType));
            opponentFleet->shipsDestroyed++;
            return SUNK;
        }
        return HIT;
    } else {
        // It's a miss
        printf("Missed!");
        if (difficulty == 1) { // In easy mode, mark misses
            grid[x][y] = 'm';
        }
        return MISS;
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
        case 'c': return &fleet->cruiser;
        case 's': return &fleet->submarine;
        case 'd': return &fleet->destroyer;
        default: return NULL;  // Invalid ship type
    }
}

// Helper function to find the name of the ship based on type
const char* getShipNameByType(char type) {
    switch (type) {
        case 'C': return "Carrier";
        case 'B': return "Battleship";
        case 'c': return "Cruiser";
        case 's': return "Submarine";
        case 'd': return "Destroyer";
        default: return "Unknown Ship";
    }
}



// used for checking if a command is right
int isValidCommand(const char *command, char pos, int row,char grid[GRID_SIZE][GRID_SIZE]) {
    return (
        (strcmp(command, "Fire") == 0 ||
         strcmp(command, "Smoke") == 0 ||
         strcmp(command, "Radar") == 0 ||
         strcmp(command, "Artillery") == 0 ||
         strcmp(command, "Torpedo") == 0) &&
        (pos >= 'A' && pos <= 'J') &&
        (row >= 1 && row <= 10) && (grid[row][pos-'A'] != 'm') && (grid[row][pos-'A'] != 'h')
    );
}


// Initialize each fleet with its ships and sizes
void initializeFleet(Fleet *fleet) {
    fleet->carrier = (Ship){'C', 5, 0};
    fleet->battleship = (Ship){'B', 4, 0};
    fleet->cruiser = (Ship){'c', 3, 0};
    fleet->submarine = (Ship){'s', 3, 0};
    fleet->destroyer = (Ship){'d', 2, 0};
    fleet->shipsDestroyed = 0;
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
    // it's first Player's turn to hit
    if(turn == 1){
        printf("Here is your grid: \n");
        printGrid(grid1);
        printf("Here is Player 2's grid: \n");
        printGrid(grid2for1);
    }else{
        printf("Here is your grid: \n");
        printGrid(grid2);
        printf("Here is Player 1's grid: \n");
        printGrid(grid1for2);
    }
}