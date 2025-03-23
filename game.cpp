#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include "math.h"

// TFT Display Pins
#define TFT_RST A4
#define TFT_RS  A3
#define TFT_CS  A5  // SS
#define TFT_SDI A2  // MOSI
#define TFT_CLK A1  // SCK
#define TFT_LED 0   // 0 if wired to +5V directly

#define TFT_BRIGHTNESS 200 

// Rotary Encoder 1 Inputs (Menu Navigation)
#define inputCLK1 4
#define inputDT1 5
#define buttonPin1 9  // Push button for encoder 1

// Rotary Encoder 2 Inputs (Selection Confirmation)
#define inputCLK2 11
#define inputDT2 12
#define buttonPin2 8  // Push button for encoder 2

// Encoder variables
int counter1 = 0; 
int counter2 = 0; 
int currentStateCLK1, previousStateCLK1;
int currentStateCLK2, previousStateCLK2;
int currentStateDT1, previousStateDT1;
int currentStateDT2, previousStateDT2;
String encdir1 = "";
String encdir2 = "";

// Encoder speed tracking
unsigned long lastEncoderTime1 = 0;
unsigned long lastEncoderTime2 = 0;
int encoderSpeed1 = 1;
int encoderSpeed2 = 1;
#define ENCODER_SPEED_THRESHOLD 100  // Time in ms to determine speed
#define MAX_SPEED_MULTIPLIER 20      // Maximum speed multiplier (extreme value)

// Game constants
#define GAME_TIME 60        // Game duration in seconds
#define COIN_APPEAR_TIME 2500  // Coin appears every 1 second
#define MAX_COINS 3         // Maximum number of coins on screen
#define BALL_RADIUS 10
#define COIN_RADIUS 4
#define BACKGROUND_COLOR COLOR_BLACK
#define SCREEN_WIDTH 176
#define SCREEN_HEIGHT 220
#define BASE_MOVEMENT_SPEED 20  // Base speed of ball movement with encoder (extreme value)
#define JUMP_SPEED 30        // Speed of rising when button is pressed (extreme value)
#define GRAVITY 30           // Speed of falling when button is released (extreme value)
#define GROUND_LEVEL 200    // Y position of the ground

// Menu variables
int menuIndex1 = 0; // 0 = YES, 1 = NO
int menuIndex2 = 0; // 0 = YES, 1 = NO
bool startGame1 = false;
bool startGame2 = false;
bool locked1 = false; // Lock state for encoder 1
bool locked2 = false; // Lock state for encoder 2

// Button states
boolean buttonState1 = HIGH;  // Default state (not pressed)
boolean buttonState2 = HIGH;  // Default state (not pressed)

// Ball positions and movement
int x1 = 40;                  // Ball 1 position X (left side)
int y1 = GROUND_LEVEL - BALL_RADIUS;  // Ball 1 position Y (on ground)
int x2 = 136;                 // Ball 2 position X (right side)
int y2 = GROUND_LEVEL - BALL_RADIUS;  // Ball 2 position Y (on ground)
int prevX1, prevY1;           // Previous Ball 1 position
int prevX2, prevY2;           // Previous Ball 2 position

// Encoder previous values for movement detection
int prevCounter1 = 0;
int prevCounter2 = 0;

// Coin variables
struct Coin {
  int x;
  int y;
  bool active;
  unsigned long creationTime;
};

Coin coins[MAX_COINS];
int coinIndex = 0;  // Index to track the next coin to create/replace
unsigned long lastCoinTime = 0;

// Score variables
int score1 = 0;
int score2 = 0;
int prevScore1 = 0;
int prevScore2 = 0;

// Timer variables
unsigned long gameStartTime = 0;
int remainingTime = GAME_TIME;
int prevRemainingTime = GAME_TIME;

// Ground line redraw timer
unsigned long lastGroundRedrawTime = 0;
#define GROUND_REDRAW_INTERVAL 100  // Redraw ground line every 100ms

// Function declarations
void showResults();
void redrawGroundLine();

// Function to reset all game variables
void resetGame() {
  // Reset scores
  score1 = 0;
  score2 = 0;
  prevScore1 = 0;
  prevScore2 = 0;

  // Reset ball positions
  x1 = 40;
  y1 = GROUND_LEVEL - BALL_RADIUS;
  x2 = 136;
  y2 = GROUND_LEVEL - BALL_RADIUS;
  prevX1 = x1;
  prevY1 = y1;
  prevX2 = x2;
  prevY2 = y2;

  // Reset coin states
  for (int i = 0; i < MAX_COINS; i++) {
    coins[i].active = false;
  }
  coinIndex = 0;
  lastCoinTime = 0;

  // Reset timers
  gameStartTime = millis();
  remainingTime = GAME_TIME;
  prevRemainingTime = GAME_TIME;

  // Reset encoder counters
  counter1 = 0;
  counter2 = 0;
  prevCounter1 = 0;
  prevCounter2 = 0;

  // Reset encoder speed tracking
  lastEncoderTime1 = 0;
  lastEncoderTime2 = 0;
  encoderSpeed1 = 1;
  encoderSpeed2 = 1;

  // Reset button states
  buttonState1 = HIGH;
  buttonState2 = HIGH;
}

void runGame();
void createCoin();
void updateScoreboard();
void initializeGameScreen();
void resetMenu();
void updateMenu();
void drawStartMenu();
void drawShapes();
void askPlayAgain();
void updatePlayAgainMenu();

// Initialize TFT object
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK, TFT_LED);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  Serial.println("Initializing...");

  // Initialize TFT Display
  tft.begin();
  tft.setOrientation(1); // Landscape
  tft.setBacklight(TFT_BRIGHTNESS);
  tft.setBackgroundColor(BACKGROUND_COLOR);
  tft.clear();
  
  // Set encoder 1 pins
  pinMode(inputCLK1, INPUT);
  pinMode(inputDT1, INPUT);
  pinMode(buttonPin1, INPUT_PULLUP);  // Internal pull-up resistor for button 1

  // Set encoder 2 pins
  pinMode(inputCLK2, INPUT);
  pinMode(inputDT2, INPUT);
  pinMode(buttonPin2, INPUT_PULLUP);  // Internal pull-up resistor for button 2

  // Read initial state of encoders
  previousStateCLK1 = digitalRead(inputCLK1);
  previousStateDT1 = digitalRead(inputDT1);
  previousStateCLK2 = digitalRead(inputCLK2);
  previousStateDT2 = digitalRead(inputDT2);

  // Initialize random seed
  randomSeed(analogRead(0));

  // Initialize coins array
  for (int i = 0; i < MAX_COINS; i++) {
    coins[i].active = false;
  }

  Serial.println("Encoders and TFT Ready!");

  // Draw initial visuals
  drawShapes();
  drawStartMenu();
}

void loop()
{
  // Read the current state of inputCLKs
  currentStateCLK1 = digitalRead(inputCLK1);
  currentStateCLK2 = digitalRead(inputCLK2);

  // Rotary Encoder 1 (Menu Navigation)
  if (!locked1 && currentStateCLK1 != previousStateCLK1) { 
    if (digitalRead(inputDT1) != currentStateCLK1) { 
      counter1++;
      encdir1 = "CW";
      menuIndex1 = (menuIndex1 + 1) % 2; // Toggle between 0 and 1
    } else {
      counter1--;
      encdir1 = "CCW";
      menuIndex1 = (menuIndex1 - 1 + 2) % 2; // Toggle between 0 and 1
    }
    Serial.print("Encoder 1 -> Direction: ");
    Serial.print(encdir1);
    Serial.print(" -- Value: ");
    Serial.println(counter1);
    updateMenu(); // Update the menu display
  } 
  previousStateCLK1 = currentStateCLK1; 

  // Rotary Encoder 2 (Selection)
  if (!locked2 && currentStateCLK2 != previousStateCLK2) { 
    if (digitalRead(inputDT2) != currentStateCLK2) { 
      counter2++;
      encdir2 = "CW";
      menuIndex2 = (menuIndex2 + 1) % 2; // Toggle between 0 and 1
    } else {
      counter2--;
      encdir2 = "CCW";
      menuIndex2 = (menuIndex2 - 1 + 2) % 2; // Toggle between 0 and 1
    }
    Serial.print("Encoder 2 -> Direction: ");
    Serial.print(encdir2);
    Serial.print(" -- Value: ");
    Serial.println(counter2);
    updateMenu(); // Update the menu display
  } 
  previousStateCLK2 = currentStateCLK2;

  // Push button for encoder 1 (Confirm selection)
  boolean newButtonState1 = digitalRead(buttonPin1);
  if (newButtonState1 != buttonState1) {
    if (newButtonState1 == LOW) { 
      Serial.println("Encoder 1 Button Pressed!");
      locked1 = true; // Lock encoder 1
      if (menuIndex1 == 0) { // YES selected
        startGame1 = true;
      } else { // NO selected
        // Do nothing, wait for both players to press NO
      }
    }
    buttonState1 = newButtonState1;
  }

  // Push button for encoder 2 (Confirm selection)
  boolean newButtonState2 = digitalRead(buttonPin2);
  if (newButtonState2 != buttonState2) {
    if (newButtonState2 == LOW) {
      Serial.println("Encoder 2 Button Pressed!");
      locked2 = true; // Lock encoder 2
      if (menuIndex2 == 0) { // YES selected
        startGame2 = true;
      } else { // NO selected
        // Do nothing, wait for both players to press NO
      }
    }
    buttonState2 = newButtonState2;
  }

  // Start the game if both players selected "YES"
  if (startGame1 && startGame2) {
    startGame1 = false; // Reset flag
    startGame2 = false;
    tft.clear();
    tft.drawText(26, 100, "Game Starts", COLOR_WHITE);
    delay(500);
    tft.clear();
    // Add game logic here
    initializeGameScreen();
    runGame();
    showResults();
    resetMenu();
   
  }

  // Display "Thank You" and reset the menu if both players selected "NO"
  if (locked1 && locked2 && menuIndex1 == 1 && menuIndex2 == 1) {
    tft.clear();
    tft.drawText(30, 100, "Thank You!", COLOR_WHITE);
    delay(2000);
    tft.clear();
    resetMenu(); // Reset the menu
  }


  // Display "Thank You" and reset the menu if both players selected "NO"
  if (locked1 && locked2 && menuIndex1 == 1 && menuIndex2 == 1)
  {
    tft.clear();
    tft.drawText(30, 100, "Thank You!", COLOR_WHITE);
    delay(2000);
    tft.clear();
    resetMenu(); // Reset the menu
  }
}

// Welcome message
void drawShapes() {
  tft.setOrientation(4);
  tft.drawRectangle(0, 0, 175, 219, COLOR_WHITE);
  tft.drawRectangle(25, 45, 150, 175, COLOR_BLACK);
  tft.drawRectangle(10, 10, 100, 60, COLOR_RED);
  tft.fillRectangle(120, 10, 170, 60, COLOR_BLUE);
  tft.drawCircle(55, 120, 30, COLOR_GREEN);
  tft.fillCircle(140, 120, 30, COLOR_YELLOW);
  tft.drawLine(10, 160, 170, 160, COLOR_YELLOW);
  tft.setFont(Terminal12x16);
  tft.drawText(10, 180, "Hello, PLAYERS", COLOR_WHITE);
  delay(300);
  tft.clear();
  tft.drawRectangle(0, 0, 175, 219, COLOR_WHITE);
  tft.drawRectangle(25, 45, 150, 175, COLOR_BLACK);
  tft.setFont(Terminal11x16);
  tft.drawText(37, 45, "Welcome to", COLOR_WHITE);
  tft.setFont(Terminal12x16);
  tft.drawText(20, 85, "HUNGRY BALLS", COLOR_DARKCYAN);
  tft.setFont(Terminal11x16);
  tft.drawText(22, 125, "Eat more coins", COLOR_YELLOW);
  tft.drawText(50, 155, ".. WIN ..", COLOR_YELLOW);
  tft.drawText(20, 190, "Time Limit: 60", COLOR_WHITE);
  delay(1500);
  tft.clear();
}

// Function to draw the Start Menu
void drawStartMenu() {
  tft.setOrientation(4);
  tft.setFont(Terminal12x16);
  tft.drawText(5, 10, "Do you want to", COLOR_WHITE);
  tft.drawText(5, 35, "START the Game?", COLOR_WHITE);
  // Draw "YES" and "NO" options
  tft.drawText(10, 80, " YES", COLOR_WHITE);
  tft.drawText(10, 120, " NO", COLOR_WHITE);
  tft.setFont(Terminal11x16);
  tft.drawText(10, 164, "Player1 -> RED", COLOR_RED);
  tft.drawText(10, 190, "Player2 -> BLUE", COLOR_BLUE);

  // Highlight the selected option for PLAYER 1
  if (menuIndex1 == 0) {
    tft.drawRectangle(5, 76, 170, 97, COLOR_RED);
  } else if (menuIndex1 == 1) {
    tft.drawRectangle(5, 116, 170, 137, COLOR_RED);
  }

  // Highlight the selected option for PLAYER 2
  if (menuIndex2 == 0) {
    tft.drawRectangle(2, 73, 173, 100, COLOR_BLUE);
  } else if (menuIndex2 == 1) {
    tft.drawRectangle(2, 113.5, 173, 140, COLOR_BLUE); 
  }
  updateMenu();
}

// Function to update the menu (without clearing the entire screen)
void updateMenu() {
  // Erase the previous rectangles by drawing black rectangles
  tft.fillRectangle(5, 76, 170, 97, COLOR_BLACK); // Erase Player 1's previous rectangle
  tft.fillRectangle(5, 116, 170, 137, COLOR_BLACK); // Erase Player 1's previous rectangle
  tft.fillRectangle(2, 73, 173, 100, COLOR_BLACK); // Erase Player 2's previous rectangle
  tft.fillRectangle(2, 113.5, 173, 140, COLOR_BLACK); // Erase Player 2's previous rectangle

  // Redraw the text (to ensure it's not overwritten by the black rectangles)
  tft.drawText(10, 80, " YES", COLOR_WHITE);
  tft.drawText(10, 120, " NO", COLOR_WHITE); 

  // Highlight the selected option for PLAYER 1
  if (menuIndex1 == 0) {
    tft.drawRectangle(5, 76, 170, 97, COLOR_RED); 
  } else if (menuIndex1 == 1) {
    tft.drawRectangle(5, 116, 170, 137, COLOR_RED);
  }

  // Highlight the selected option for PLAYER 2
  if (menuIndex2 == 0) {
    tft.drawRectangle(2, 73, 173, 100, COLOR_BLUE); 
  } else if (menuIndex2 == 1) {
    tft.drawRectangle(2, 113.5, 173, 140, COLOR_BLUE);
  }
}

// Function to reset the menu
void resetMenu() {
  locked1 = false; // Unlock encoder 1
  locked2 = false; // Unlock encoder 2
  menuIndex1 = 0;  // Reset menu index for encoder 1
  menuIndex2 = 0;  // Reset menu index for encoder 2
  drawStartMenu(); // Redraw the start menu
}

// Initialize the game screen with static elements
void initializeGameScreen() 
{
  // Draw the scoreboard background and static elements
  tft.setFont(Terminal6x8);
  tft.drawText(10, 5, "P1:", COLOR_RED);
  tft.drawText(64, 5, "TIME:", COLOR_WHITE);
  tft.drawText(136, 5, "P2:", COLOR_BLUE);
  
  // Draw separator line
  tft.drawLine(0, 20, 220, 20, COLOR_WHITE);
  
  // Draw ground line
  tft.drawLine(0, GROUND_LEVEL, SCREEN_WIDTH, GROUND_LEVEL, COLOR_WHITE);

  updateScoreboard();
}

// Update only the changing parts of the scoreboard
void updateScoreboard() {
  tft.setFont(Terminal6x8);
  
  // Update timer if changed
  if (remainingTime != prevRemainingTime) {
    // Clear previous time
    tft.fillRectangle(120, 5, 130, 15, BACKGROUND_COLOR);
    
    // Draw new time
    char timeStr[5];
    sprintf(timeStr, "%2d", remainingTime);
    tft.drawText(103, 5, timeStr, COLOR_WHITE);
    
    prevRemainingTime = remainingTime;
  }
  
  // Update P1 score if changed
  if (score1 != prevScore1) {
    // Clear previous score
    tft.fillRectangle(30, 5, 50, 15, BACKGROUND_COLOR);
    
    // Draw new score
    char score1Str[5];
    sprintf(score1Str, "%2d", score1);
    tft.drawText(30, 5, score1Str, COLOR_RED);
    
    prevScore1 = score1;
  }
  
  // Update P2 score if changed
  if (score2 != prevScore2) {
    // Clear previous score
    tft.fillRectangle(156, 5, 168, 15, BACKGROUND_COLOR);
    
    // Draw new score
    char score2Str[5];
    sprintf(score2Str, "%2d", score2);
    tft.drawText(157, 5, score2Str, COLOR_BLUE);
    
    prevScore2 = score2;
  }
}

// Create a new coin
void createCoin() {
  // Erase the old coin at this index if it was active
  if (coins[coinIndex].active) {
    tft.fillCircle(coins[coinIndex].x, coins[coinIndex].y, COIN_RADIUS, BACKGROUND_COLOR);
  }
  
  // Create a new coin
  coins[coinIndex].x = random(20, 160);
  coins[coinIndex].y = random(50, 180);
  coins[coinIndex].active = true;
  coins[coinIndex].creationTime = millis();
  
  // Draw the new coin
  tft.fillCircle(coins[coinIndex].x, coins[coinIndex].y, COIN_RADIUS, COLOR_YELLOW);
  
  // Move to the next coin slot (circular buffer)
  coinIndex = (coinIndex + 1) % MAX_COINS;
}

// Calculate encoder speed multiplier based on time between rotations
int calculateSpeedMultiplier(unsigned long lastTime) {
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - lastTime;
  
  // If this is the first rotation or it's been a long time, use base speed
  if (lastTime == 0 || timeDiff > 1000) {
    return 1;
  }
  
  // The faster the rotation (smaller timeDiff), the higher the multiplier
  if (timeDiff < ENCODER_SPEED_THRESHOLD) {
    // Map time difference to speed multiplier (faster = higher multiplier)
    int multiplier = map(timeDiff, 0, ENCODER_SPEED_THRESHOLD, MAX_SPEED_MULTIPLIER, 1);
    return multiplier;
  }
  
  return 1; // Default to base speed
}

// Function to redraw the ground line
void redrawGroundLine() {
  tft.drawLine(0, GROUND_LEVEL, SCREEN_WIDTH, GROUND_LEVEL, COLOR_WHITE);
}

// Main game loop
void runGame() {
  unsigned long currentTime;
  
  // Initialize previous positions
  prevX1 = x1;
  prevY1 = y1;
  prevX2 = x2;
  prevY2 = y2;
  
  // Draw initial ball positions
  tft.fillCircle(x1, y1, BALL_RADIUS, COLOR_RED);
  tft.fillCircle(x2, y2, BALL_RADIUS, COLOR_BLUE);
  
  // Game loop runs until time is up
  while (remainingTime > 0) {
    currentTime = millis();
    
    // Update remaining time
    remainingTime = GAME_TIME - ((currentTime - gameStartTime) / 1000);
    
    // Read encoder states for movement
    currentStateCLK1 = digitalRead(inputCLK1);
    currentStateDT1 = digitalRead(inputDT1);
    currentStateCLK2 = digitalRead(inputCLK2);
    currentStateDT2 = digitalRead(inputDT2);
    
    // Encoder 1 (Player 1) movement - improved handling
    if (currentStateCLK1 != previousStateCLK1 || currentStateDT1 != previousStateDT1) { 
      // Calculate speed multiplier based on how quickly encoder is turned
      encoderSpeed1 = calculateSpeedMultiplier(lastEncoderTime1);
      lastEncoderTime1 = currentTime;
      
      // Determine direction based on both CLK and DT signals
      if (currentStateCLK1 != previousStateCLK1) {
        if (currentStateDT1 != currentStateCLK1) { 
          counter1++;
        } else {
          counter1--;
        }
      } else if (currentStateDT1 != previousStateDT1) {
        if (currentStateCLK1 == currentStateDT1) {
          counter1++;
        } else {
          counter1--;
        }
      }
    }
    previousStateCLK1 = currentStateCLK1;
    previousStateDT1 = currentStateDT1;
    
    // Encoder 2 (Player 2) movement - improved handling
    if (currentStateCLK2 != previousStateCLK2 || currentStateDT2 != previousStateDT2) { 
      // Calculate speed multiplier based on how quickly encoder is turned
      encoderSpeed2 = calculateSpeedMultiplier(lastEncoderTime2);
      lastEncoderTime2 = currentTime;
      
      // Determine direction based on both CLK and DT signals
      if (currentStateCLK2 != previousStateCLK2) {
        if (currentStateDT2 != currentStateCLK2) { 
          counter2++;
        } else {
          counter2--;
        }
      } else if (currentStateDT2 != previousStateDT2) {
        if (currentStateCLK2 == currentStateDT2) {
          counter2++;
        } else {
          counter2--;
        }
      }
    }
    previousStateCLK2 = currentStateCLK2;
    previousStateDT2 = currentStateDT2;
    
    // Check button states for jumping
    buttonState1 = digitalRead(buttonPin1);
    buttonState2 = digitalRead(buttonPin2);
    
    // Create a new coin every second
    if (currentTime - lastCoinTime > COIN_APPEAR_TIME) {
      createCoin();
      lastCoinTime = currentTime;
    }
    
    // Update ball positions based on encoder movement
    if (counter1 != prevCounter1) {
      // Move ball 1 based on encoder direction and speed
      int moveX = (counter1 - prevCounter1) * BASE_MOVEMENT_SPEED * encoderSpeed1;
      
      // Limit the maximum movement per frame to prevent extreme jumps
      if (moveX > 100) moveX = 100;
      if (moveX < -100) moveX = -100;
      
      int newX = x1 + moveX;
      
      // Keep ball within screen boundaries
      if (newX >= BALL_RADIUS && newX <= SCREEN_WIDTH - BALL_RADIUS) {
        x1 = newX;
      } else if (newX < BALL_RADIUS) {
        x1 = BALL_RADIUS;
      } else if (newX > SCREEN_WIDTH - BALL_RADIUS) {
        x1 = SCREEN_WIDTH - BALL_RADIUS;
      }
      
      prevCounter1 = counter1;
      
      // Debug output
      Serial.print("P1 Move: ");
      Serial.print(moveX);
      Serial.print(" Speed Multiplier: ");
      Serial.println(encoderSpeed1);
    }
    
    if (counter2 != prevCounter2) {
      // Move ball 2 based on encoder direction and speed
      int moveX = (counter2 - prevCounter2) * BASE_MOVEMENT_SPEED * encoderSpeed2;
      
      // Limit the maximum movement per frame to prevent extreme jumps
      if (moveX > 100) moveX = 100;
      if (moveX < -100) moveX = -100;
      
      int newX = x2 + moveX;
      
      // Keep ball within screen boundaries
      if (newX >= BALL_RADIUS && newX <= SCREEN_WIDTH - BALL_RADIUS) {
        x2 = newX;
      } else if (newX < BALL_RADIUS) {
        x2 = BALL_RADIUS;
      } else if (newX > SCREEN_WIDTH - BALL_RADIUS) {
        x2 = SCREEN_WIDTH - BALL_RADIUS;
      }
      
      prevCounter2 = counter2;
      
      // Debug output
      Serial.print("P2 Move: ");
      Serial.print(moveX);
      Serial.print(" Speed Multiplier: ");
      Serial.println(encoderSpeed2);
    }
    
    // Update vertical position based on button state (jumping)
    
    // Player 1 jump logic
    if (buttonState1 == LOW) {
      // Button is pressed, move ball up
      y1 -= JUMP_SPEED;
      
      // Don't let the ball go above the top of the screen
      if (y1 < BALL_RADIUS + 25) { // +25 to leave space for scoreboard
        y1 = BALL_RADIUS + 25;
      }
    } else {
      // Button is released, apply gravity
      y1 += GRAVITY;
      
      // Don't let the ball go below the ground
      if (y1 > GROUND_LEVEL - BALL_RADIUS) {
        y1 = GROUND_LEVEL - BALL_RADIUS;
      }
    }
    
    // Player 2 jump logic
    if (buttonState2 == LOW) {
      // Button is pressed, move ball up
      y2 -= JUMP_SPEED;
      
      // Don't let the ball go above the top of the screen
      if (y2 < BALL_RADIUS + 25) { // +25 to leave space for scoreboard
        y2 = BALL_RADIUS + 25;
      }
    } else {
      // Button is released, apply gravity
      y2 += GRAVITY;
      
      // Don't let the ball go below the ground
      if (y2 > GROUND_LEVEL - BALL_RADIUS) {
        y2 = GROUND_LEVEL - BALL_RADIUS;
      }
    }
    
    // Check for coin collection
    for (int i = 0; i < MAX_COINS; i++) {
      if (coins[i].active) {
        // Check if ball 1 collected the coin
        if (sqrt(pow(x1 - coins[i].x, 2) + pow(y1 - coins[i].y, 2)) < (BALL_RADIUS + COIN_RADIUS)) {
          score1++;
          coins[i].active = false;
          tft.fillCircle(coins[i].x, coins[i].y, COIN_RADIUS, BACKGROUND_COLOR);
        }
        
        // Check if ball 2 collected the coin
        if (sqrt(pow(x2 - coins[i].x, 2) + pow(y2 - coins[i].y, 2)) < (BALL_RADIUS + COIN_RADIUS)) {
          score2++;
          coins[i].active = false;
          tft.fillCircle(coins[i].x, coins[i].y, COIN_RADIUS, BACKGROUND_COLOR);
        }
      }
    }
    
    // Update the scoreboard if needed
    updateScoreboard();
    
    // Periodically redraw the ground line to ensure it doesn't get erased
    if (currentTime - lastGroundRedrawTime > GROUND_REDRAW_INTERVAL) {
      redrawGroundLine();
      lastGroundRedrawTime = currentTime;
    }
    
    // Only redraw balls if they've moved
    if (x1 != prevX1 || y1 != prevY1) {
      // Erase previous ball position
      tft.fillCircle(prevX1, prevY1, BALL_RADIUS, BACKGROUND_COLOR);
      
      // Draw new ball position
      tft.fillCircle(x1, y1, BALL_RADIUS, COLOR_RED);
      
      // Update previous position
      prevX1 = x1;
      prevY1 = y1;
      
      // Redraw ground line if ball was near it
      if (prevY1 + BALL_RADIUS >= GROUND_LEVEL - 2 || y1 + BALL_RADIUS >= GROUND_LEVEL - 2) {
        redrawGroundLine();
      }
    }
    
    if (x2 != prevX2 || y2 != prevY2) {
      // Erase previous ball position
      tft.fillCircle(prevX2, prevY2, BALL_RADIUS, BACKGROUND_COLOR);
      
      // Draw new ball position
      tft.fillCircle(x2, y2, BALL_RADIUS, COLOR_BLUE);
      
      // Update previous position
      prevX2 = x2;
      prevY2 = y2;
      
      // Redraw ground line if ball was near it
      if (prevY2 + BALL_RADIUS >= GROUND_LEVEL - 2 || y2 + BALL_RADIUS >= GROUND_LEVEL - 2) {
        redrawGroundLine();
      }
    }
    
    // Small delay to control game speed
    delay(0.5); // Minimal delay for maximum game speed
  }
}

// Modify the showResults() function to add a restart option
void showResults() {
  tft.clear();
  tft.setFont(Terminal12x16);
  
  tft.drawText(40, 40, "GAME OVER", COLOR_WHITE);
  
  // Display scores
  char score1Str[20];
  sprintf(score1Str, "P1: %d", score1);
  tft.drawText(40, 80, score1Str, COLOR_RED);
  
  char score2Str[20];
  sprintf(score2Str, "P2: %d", score2);
  tft.drawText(40, 110, score2Str, COLOR_BLUE);
  
  // Display winner
  if (score1 > score2) {
    tft.drawText(40, 150, "P1 WINS!", COLOR_RED);
  } else if (score2 > score1) {
    tft.drawText(40, 150, "P2 WINS!", COLOR_BLUE);
  } else {
    tft.drawText(40, 150, "IT'S A TIE!", COLOR_WHITE);
  }
  
  delay(3000); // Show results for 3 seconds
  
  // Ask if players want to play again
  askPlayAgain();
}

// Add a new function to ask players if they want to play again
void askPlayAgain() {
  tft.clear();
  
  // Reset encoder states for new input
  locked1 = false;
  locked2 = false;
  menuIndex1 = 0;
  menuIndex2 = 0;
  startGame1 = false;
  startGame2 = false;
  
  tft.setFont(Terminal12x16);
  tft.drawText(5, 10, "Play Again?", COLOR_WHITE);
  
  // Draw "YES" and "NO" options
  tft.drawText(10, 80, " YES", COLOR_WHITE);
  tft.drawText(10, 120, " NO", COLOR_WHITE);
  
  // Highlight the selected option for PLAYER 1
  if (menuIndex1 == 0) {
    tft.drawRectangle(5, 76, 170, 97, COLOR_RED);
  } else if (menuIndex1 == 1) {
    tft.drawRectangle(5, 116, 170, 137, COLOR_RED);
  }

  // Highlight the selected option for PLAYER 2
  if (menuIndex2 == 0) {
    tft.drawRectangle(2, 73, 173, 100, COLOR_BLUE);
  } else if (menuIndex2 == 1) {
    tft.drawRectangle(2, 113.5, 173, 140, COLOR_BLUE); 
  }
  
  // Wait for player inputs
  boolean playAgainDecided = false;
  
  while (!playAgainDecided) {
    // Read the current state of inputCLKs
    currentStateCLK1 = digitalRead(inputCLK1);
    currentStateCLK2 = digitalRead(inputCLK2);

    // Rotary Encoder 1 (Menu Navigation)
    if (!locked1 && currentStateCLK1 != previousStateCLK1) { 
      if (digitalRead(inputDT1) != currentStateCLK1) { 
        menuIndex1 = (menuIndex1 + 1) % 2; // Toggle between 0 and 1
      } else {
        menuIndex1 = (menuIndex1 - 1 + 2) % 2; // Toggle between 0 and 1
      }
      // Update menu display
      updatePlayAgainMenu();
    } 
    previousStateCLK1 = currentStateCLK1; 

    // Rotary Encoder 2 (Selection)
    if (!locked2 && currentStateCLK2 != previousStateCLK2) { 
      if (digitalRead(inputDT2) != currentStateCLK2) { 
        menuIndex2 = (menuIndex2 + 1) % 2; // Toggle between 0 and 1
      } else {
        menuIndex2 = (menuIndex2 - 1 + 2) % 2; // Toggle between 0 and 1
      }
      // Update menu display
      updatePlayAgainMenu();
    } 
    previousStateCLK2 = currentStateCLK2;

    // Push button for encoder 1 (Confirm selection)
    boolean newButtonState1 = digitalRead(buttonPin1);
    if (newButtonState1 != buttonState1) {
      if (newButtonState1 == LOW) { 
        locked1 = true; // Lock encoder 1
        if (menuIndex1 == 0) { // YES selected
          startGame1 = true;
        }
      }
      buttonState1 = newButtonState1;
    }

    // Push button for encoder 2 (Confirm selection)
    boolean newButtonState2 = digitalRead(buttonPin2);
    if (newButtonState2 != buttonState2) {
      if (newButtonState2 == LOW) {
        locked2 = true; // Lock encoder 2
        if (menuIndex2 == 0) { // YES selected
          startGame2 = true;
        }
      }
      buttonState2 = newButtonState2;
    }

    // Start a new game if both players selected "YES"
    if (startGame1 && startGame2) {
      playAgainDecided = true;
      tft.clear();
      tft.drawText(26, 100, "Game Starts", COLOR_WHITE);
      delay(500);
      tft.clear();
      
      // Reset game variables
      resetGame();
      
      // Initialize game screen and start a new game
      gameStartTime = millis();
      initializeGameScreen();
      runGame();
      showResults(); // This will recursively call askPlayAgain after the game
      return; // Exit this function after the recursive call returns
    }

    // Return to main menu if either player selected "NO"
    if ((locked1 && menuIndex1 == 1) || (locked2 && menuIndex2 == 1)) {
      playAgainDecided = true;
      tft.clear();
      resetMenu(); // Reset the menu and return to main menu
      return;
    }
    
    delay(10); // Small delay to prevent CPU hogging
  }
}

// Add a function to update the play again menu
void updatePlayAgainMenu() {
  // Erase the previous rectangles by drawing black rectangles
  tft.fillRectangle(5, 76, 170, 97, COLOR_BLACK); // Erase Player 1's previous rectangle
  tft.fillRectangle(5, 116, 170, 137, COLOR_BLACK); // Erase Player 1's previous rectangle
  tft.fillRectangle(2, 73, 173, 100, COLOR_BLACK); // Erase Player 2's previous rectangle
  tft.fillRectangle(2, 113.5, 173, 140, COLOR_BLACK); // Erase Player 2's previous rectangle

  // Redraw the text
  tft.drawText(10, 80, " YES", COLOR_WHITE);
  tft.drawText(10, 120, " NO", COLOR_WHITE); 

  // Highlight the selected option for PLAYER 1
  if (menuIndex1 == 0) {
    tft.drawRectangle(5, 76, 170, 97, COLOR_RED); 
  } else if (menuIndex1 == 1) {
    tft.drawRectangle(5, 116, 170, 137, COLOR_RED);
  }

  // Highlight the selected option for PLAYER 2
  if (menuIndex2 == 0) {
    tft.drawRectangle(2, 73, 173, 100, COLOR_BLUE); 
  } else if (menuIndex2 == 1) {
    tft.drawRectangle(2, 113.5, 173, 140, COLOR_BLUE);
  }
}
