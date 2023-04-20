#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <stdlib.h>
#include <time.h>

#if defined(ARDUINO_FEATHER_ESP32) // Feather Huzzah32
  #define TFT_CS         14
  #define TFT_RST        15
  #define TFT_DC         32

#elif defined(ESP8266)
  #define TFT_CS         4
  #define TFT_RST        16                                            
  #define TFT_DC         5

#else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS        10
  #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         8
#endif

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

//LCD SETUP IS FINISHED

//task struct
typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

//erase textbox function
void erase_txt(){

  tft.fillRect(11,121,108,38,ST77XX_BLACK);

  return;
}

int enemy_HP = 100;
int player_HP = 100;

//attack function
void attack_fct(int type){

  if(type == 1){
    enemy_HP = ((enemy_HP - 20) <= 0)? 0 : enemy_HP - 20;
    tft.drawFastHLine((10 + enemy_HP),5,20,ST77XX_RED);
    tft.drawFastHLine((10 + enemy_HP),6,20,ST77XX_RED); 
  }
  else if(type == 2){
    enemy_HP = ((enemy_HP - 15) <= 0)? 0 : enemy_HP - 15;
    tft.drawFastHLine((10 + enemy_HP),5,15,ST77XX_RED);
    tft.drawFastHLine((10 + enemy_HP),6,15,ST77XX_RED); 
  }
  else if(type == 3){
    enemy_HP = ((enemy_HP - 15) <= 0)? 0 : enemy_HP - 10;
    tft.drawFastHLine((10 + enemy_HP),5,10,ST77XX_RED);
    tft.drawFastHLine((10 + enemy_HP),6,10,ST77XX_RED); 
  }

  return;
}

//enemy attack function
void enemy_fct(){
  
    player_HP = ((player_HP - 15) <= 0)? 0 : player_HP - 15;
    tft.drawFastHLine((10 + player_HP),112,15,ST77XX_RED);
    tft.drawFastHLine((10 + player_HP),111,15,ST77XX_RED); 
    
  return;
}

//creation of task object
const unsigned short tasksNum = 2; //FIXME
task tasks[tasksNum];

//global variables needed for the game
int xValue = 0 ;
int yValue = 0 ; 
int bValue = 0 ;
int potion_cnt = 2;
int type_attack = 0; //type of player attack selected, will be used as flag for animations as well as calculating damage, 1 for slime, 2 for chomp, 3 for poison.
int attack_rep = 0;//variable used to make state hold for a certain duration
int enemy_rep = 0;
int potion_rep = 0;
int enemy_flag = 0; //flag to tell when the enemy's turn is under way

//ENUMERATION OF MENU SELECT TASK
enum MS_states{MS_start, MS_FH, MS_IH, MS_SH, MS_RH, MS_Fslimebub, MS_Fpoison, MS_Fchomp, MS_Fback, MS_attack, MS_enemyTurn, MS_Ipotion, MS_Iback, MS_heal, MS_WIN, MS_LOSE} MS_state;

//ENUMERATION OF EFFECT TASK
enum ET_states{ET_start, ET_wait, ET_part1, ET_part2, ET_part3} ET_state;

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(30,130);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.println(" Fight    Item");

  tft.setCursor(30,140);
  tft.setTextSize(1);
  tft.println(" Swap     Run");

  tft.drawRect(10,120,110,40,ST77XX_WHITE);
  //clear rectangle
  //tft.fillRect(11,121,108,38,ST77XX_BLACK);

     //player
    tft.fillCircle(35,80,20,ST77XX_GREEN);//body
    tft.fillCircle(55,55,16,ST77XX_GREEN);//head
    tft.fillCircle(50,95,10,ST77XX_GREEN); //arm
    tft.fillCircle(65,50,5,ST77XX_WHITE); //eye
    tft.fillCircle(67,48,3,ST77XX_CYAN); //pupil
    tft.fillTriangle(30,65,5,105,30,95,ST77XX_GREEN);//tail
    tft.fillTriangle(45,65,35,63,37,73,ST77XX_ORANGE);//spike1
    tft.fillTriangle(33,74,22,72,25,82,ST77XX_ORANGE);//spike2
    tft.fillTriangle(23,87,12,85,15,95,ST77XX_ORANGE);//spike2

    //enemy
    tft.fillCircle(100,40,13,ST77XX_RED); //body
    tft.fillCircle(93,53,5,ST77XX_RED); //foot
    tft.fillCircle(107,53,5,ST77XX_RED); //foot
    tft.fillCircle(100,43,5,ST77XX_WHITE); //eye
    tft.fillCircle(99,44,3,ST77XX_YELLOW); //pupil
    tft.fillTriangle(100,15,88,35,112,35,ST77XX_ORANGE);//horn

  //character health
  tft.drawFastHLine(10,112,100,ST77XX_GREEN);
  tft.drawFastHLine(10,111,100,ST77XX_GREEN);

  //enemy health
  tft.drawFastHLine(10,5,100,ST77XX_GREEN);
  tft.drawFastHLine(10,6,100,ST77XX_GREEN);

  pinMode(2,INPUT);
  digitalWrite(2,HIGH);

  pinMode(3, OUTPUT);

  //set up MS tick function
  tasks[0].state = MS_start; 
  tasks[0].period = 200;
  tasks[0].elapsedTime = 0;
  tasks[0].TickFct = &MS_TickFct;

  //set up ET tick function
  tasks[1].state = ET_start; 
  tasks[1].period = 500;
  tasks[1].elapsedTime = 0;
  tasks[1].TickFct = &ET_TickFct;
}

int MS_TickFct(int state){

  switch(state){//transitions for Menu select task
    case MS_start:
      state = MS_FH;
    break;

    case MS_FH:
      if(xValue >= 500){ //joystick right
        state = MS_IH;
        tft.setCursor(30,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else if(yValue >= 500){ //joystick down
        state = MS_SH;
        tft.setCursor(30,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else if(bValue == 0){
        erase_txt();
        tft.setCursor(10,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(" Slime   Chomp");

        tft.setCursor(10,140);
        tft.println(" Poison  Back");
        
        state = MS_Fslimebub;
        
      }
      else{
        state = MS_FH;
      }
    break;

    case MS_IH:
      if(xValue <= 50){
        state = MS_FH;
        tft.setCursor(80,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else if(yValue >= 500){
        state = MS_RH;
        tft.setCursor(80,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else if(bValue == 0){
        erase_txt();
        tft.setCursor(10,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(" Potion   Back");
        
        state = MS_Ipotion;
      }
      else{
        state = MS_IH;
      }
    break;

    case MS_SH:
      if(xValue >= 500){
        state = MS_RH;
        tft.setCursor(30,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else if(yValue <= 50){
        state = MS_FH;
        tft.setCursor(30,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else{
        state = MS_SH;
      }
    break;

    case MS_RH:
        if(xValue <= 50){
        state = MS_SH;
        tft.setCursor(80,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else if(yValue <= 50){
        state = MS_IH;
        tft.setCursor(80,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
      }
      else if(bValue == 0){
        state = MS_LOSE;
        tft.fillScreen(ST77XX_BLACK);
      }
      else{
        state = MS_RH;
      }
    break;

    case MS_Fslimebub:
      if(xValue >= 500){
        tft.setCursor(10,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fchomp;
      }
      else if(yValue >= 500){
        tft.setCursor(10,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fpoison;
      }
      else if(bValue == 0){
        state = MS_attack;
        type_attack = 1;
        attack_fct(type_attack);
        erase_txt();
        tft.setCursor(12,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println("Gumpy used");
        tft.setCursor(12,140);
        tft.println("Slime Bubble!!!");
      }
      else{
        state = MS_Fslimebub;
      }
    break;

    case MS_Fchomp:
      if(xValue <= 50){
        tft.setCursor(55,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fslimebub;
      }
      else if(yValue >= 500){
        tft.setCursor(55,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fback;
      }
      else if(bValue == 0){
        state = MS_attack;
        type_attack = 2;
        attack_fct(type_attack);
        erase_txt();
        tft.setCursor(12,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println("Gumpy used");
        tft.setCursor(12,140);
        tft.println("Crushing Chomp!!!");
      }
      else{
        state = MS_Fchomp;
      }
    break;

    case MS_Fpoison:
      if(xValue >= 500){
        tft.setCursor(10,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fback;
      }
      else if(yValue <= 50){
        tft.setCursor(10,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fslimebub;
      }
      else if(bValue == 0){
        state = MS_attack;
        type_attack = 3;
        attack_fct(type_attack);
        erase_txt();
        tft.setCursor(12,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println("Gumpy used");
        tft.setCursor(12,140);
        tft.println("Poison Spit!!!");
      }
      else{

        state = MS_Fpoison;
      }
    break;

    case MS_Fback:
      if(xValue <= 50){
        tft.setCursor(55,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fpoison;
      }
      else if(yValue <= 50){
        tft.setCursor(55,140);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Fchomp;
      }
      else if(bValue == 0){
        state = MS_FH;
        
        erase_txt();
        tft.setCursor(30,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(" Fight    Item");

        tft.setCursor(30,140);
        tft.println(" Swap     Run");
      }
      else{

        state = MS_Fback;
      }
    break;

    case MS_attack:
      if(attack_rep < 10){
        state = MS_attack;
      }
      else{
        state = MS_enemyTurn;
        attack_rep = 0;
        type_attack = 0;
        
        erase_txt();
        tft.setCursor(12,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println("Bumper used");
        tft.setCursor(12,140);
        tft.println("Ember Flurry!!!");
        
        if(enemy_HP <= 0){//if the player has won
          state = MS_WIN;
          tft.fillScreen(ST77XX_BLACK);
          enemy_flag = 0;
        }
      }
    break;

    case MS_enemyTurn:
        enemy_flag = 1;
        if(enemy_rep < 10){
          state = MS_enemyTurn;
        }
        else{
            state = MS_FH;
            enemy_rep = 0;
            enemy_flag = 0;
            enemy_fct();

            erase_txt();
            tft.setCursor(30,130);
            tft.setTextColor(ST77XX_WHITE);
            tft.println(" Fight    Item");

            tft.setCursor(30,140);
            tft.println(" Swap     Run");

            if(player_HP <= 0){
              state = MS_LOSE;
              tft.fillScreen(ST77XX_BLACK);
            }
        }
        
    break;

    case MS_Ipotion:
        if(xValue >= 500){
        tft.setCursor(10,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
          state = MS_Iback;
        }
        else if(bValue == 0){
          erase_txt();

          state = MS_heal;

          
          
          if(potion_cnt > 0){
            tft.setCursor(12,130);
            tft.setTextColor(ST77XX_WHITE);
            tft.println("Gumpy used");
            tft.setCursor(12,140);
            tft.println("a Potion!!!");

            player_HP = ((player_HP + 25) >= 100)? 100 : (player_HP + 25);
            tft.drawFastHLine((player_HP - 15),112,25,ST77XX_GREEN);
            tft.drawFastHLine((player_HP - 15),111,25,ST77XX_GREEN);
            --potion_cnt;
          }
          else{
            tft.setCursor(12,130);
            tft.setTextColor(ST77XX_WHITE);
            tft.println("Gumpy ran out");
            tft.setCursor(12,140);
            tft.println("of Potions!");
          }
        }
        else{
          state = MS_Ipotion;
        }
        break;
    
    case MS_heal:
        if(potion_rep < 10){
          state = MS_heal;
        }
        else{
           potion_rep = 0;
           state = MS_enemyTurn;
           
           erase_txt();
           tft.setCursor(12,130);
           tft.setTextColor(ST77XX_WHITE);
           tft.println("Bumper used");
           tft.setCursor(12,140);
           tft.println("Ember Flurry!!!");
        }
    break;

    case MS_Iback:
      if(xValue <= 50){
        tft.setCursor(55,130);
        tft.setTextColor(ST77XX_BLACK);
        tft.println(">");
        state = MS_Ipotion;
      }
      else if(bValue == 0){
        state = MS_FH;
        
        erase_txt();
        tft.setCursor(30,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(" Fight    Item");

        tft.setCursor(30,140);
        tft.println(" Swap     Run");
      }
      else{
        state = MS_Iback;
      }
    break;

    case MS_WIN:
        state = MS_WIN;
    break;

    case MS_LOSE:
        state = MS_LOSE;
    break;
    
    default:
      state = MS_FH;
    break;
  }

  switch(state){ //menu state actions

    case MS_start:
    break; 

    case MS_FH:
        tft.setCursor(30,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_SH:
        tft.setCursor(30,140);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_IH:
        tft.setCursor(80,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_RH:
        tft.setCursor(80,140);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_Fslimebub:
        tft.setCursor(10,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_Fchomp:
        tft.setCursor(55,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_Fpoison:
        tft.setCursor(10,140);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");

    break;

    case MS_Fback:
        tft.setCursor(55,140);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_attack:
        ++attack_rep;
    break;

    case MS_enemyTurn:
        ++enemy_rep;
    break;

    case MS_Ipotion:
        tft.setCursor(10,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_heal:
        ++potion_rep;
    break;

    case MS_Iback:
        tft.setCursor(55,130);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(">");
    break;

    case MS_WIN:
        tft.setCursor(50,100);
        tft.setTextColor(ST77XX_GREEN);
         tft.setTextSize(3);
        tft.println("YOU WIN!");
    break;

    case MS_LOSE:
        tft.setCursor(50,100);
        tft.setTextColor(ST77XX_RED);
        tft.setTextSize(3);
        tft.println("YOU LOSE :(");
    break;
    
  }

  return state;
}

//EFFECT TICK FUNCTION
int ET_TickFct(int state){

  switch(state){//state transistions for Effect Task
    case ET_start:
        state = ET_wait;
    break;

    case ET_wait:
        if((type_attack != 0) || (enemy_flag == 1 )){
          state = ET_part1;
        }
        else{
          state = ET_wait;
        }
    break;

    case ET_part1:
        state = ET_part2;
    break;

    case ET_part2:
        state = ET_part3;
    break;

    case ET_part3:
        noTone(3);
        state = ET_wait;
    break;

    default:
        state = ET_wait;
    break;
  }//end of transitions for effect task

  switch(state){//state actions for effect task
    case ET_start:
    break;

    case ET_wait:
        if(type_attack == 1){
          tft.fillCircle(15,30,7, ST77XX_BLACK);
          tft.fillCircle(30,15,3, ST77XX_BLACK);
          tft.fillCircle(33,27,5, ST77XX_BLACK);
        }
        else if(type_attack == 2){
          tft.fillTriangle(15,15,25,15,20,25,ST77XX_BLACK);//chomp teeth
          tft.fillTriangle(25,15,35,15,30,25,ST77XX_BLACK);
          tft.fillTriangle(35,15,45,15,40,25,ST77XX_BLACK);

          tft.fillTriangle(20,37,25,27,30,37,ST77XX_BLACK);
          tft.fillTriangle(30,37,35,27,40,37,ST77XX_BLACK);
        }
        else if(type_attack == 3){
          tft.fillCircle(20,25,10, ST77XX_BLACK);
          tft.fillCircle(22,35,8, ST77XX_BLACK);
          tft.fillCircle(30,20,6, ST77XX_BLACK); 
        }
        else if(enemy_flag == 1){
          tft.invertDisplay(false);
        }
    break;

    case ET_part1:
      if(type_attack == 1){
        tone(3, 139); 
        tft.fillCircle(15,30,7, ST77XX_YELLOW);
        tft.fillCircle(30,15,3, ST77XX_YELLOW);
        tft.fillCircle(33,27,5, ST77XX_YELLOW);
      }
      else if(type_attack == 2){
        tone(3, 932); 
        tft.fillTriangle(15,15,25,15,20,25,ST77XX_WHITE);//chomp teeth
        tft.fillTriangle(25,15,35,15,30,25,ST77XX_WHITE);
        tft.fillTriangle(35,15,45,15,40,25,ST77XX_WHITE);

        tft.fillTriangle(20,37,25,27,30,37,ST77XX_WHITE);
        tft.fillTriangle(30,37,35,27,40,37,ST77XX_WHITE);
      }
      else if(type_attack == 3){
        tone(3, 208); 
        tft.fillCircle(20,25,10, ST77XX_MAGENTA);
        tft.fillCircle(22,35,8, ST77XX_MAGENTA);
        tft.fillCircle(30,20,6, ST77XX_MAGENTA);
      }
      else if(enemy_flag == 1){
        tone(3, 494); 
        tft.invertDisplay(true);
      }
    break;

    case ET_part2:
      if(type_attack == 1){
        tone(3, 147); 
        tft.fillCircle(15,30,7, ST77XX_BLACK);
        tft.fillCircle(30,15,3, ST77XX_BLACK);
        tft.fillCircle(33,27,5, ST77XX_BLACK);

        tft.fillCircle(15,20,7, ST77XX_YELLOW);
        tft.fillCircle(20,35,3, ST77XX_YELLOW);
        tft.fillCircle(33,37,5, ST77XX_YELLOW);
      }
      else if(type_attack == 2){
        tone(3, 880); 
        
        tft.fillTriangle(20,37,25,27,30,37,ST77XX_BLACK);
        tft.fillTriangle(30,37,35,27,40,37,ST77XX_BLACK);

        tft.fillTriangle(20,32,25,22,30,32,ST77XX_WHITE);
        tft.fillTriangle(30,32,35,22,40,32,ST77XX_WHITE);
      }
      else if(type_attack == 3){
        tone(3, 196); 
        tft.fillCircle(20,25,10, ST77XX_BLACK);
        tft.fillCircle(22,35,8, ST77XX_BLACK);
        tft.fillCircle(30,20,6, ST77XX_BLACK);
        
        tft.fillCircle(20,25,5, ST77XX_MAGENTA);
        tft.fillCircle(22,35,8, ST77XX_MAGENTA);
        tft.fillCircle(30,20,4, ST77XX_MAGENTA);
      }
      else if(enemy_flag == 1){
        tone(3, 587); 
        tft.invertDisplay(false);
      }
    break;

    case ET_part3:
      if(type_attack == 1){
        tone(3, 156); 
        tft.fillCircle(15,20,7, ST77XX_BLACK);
        tft.fillCircle(20,35,3, ST77XX_BLACK);
        tft.fillCircle(33,37,5, ST77XX_BLACK);
        
        tft.fillCircle(15,30,7, ST77XX_YELLOW);
        tft.fillCircle(30,15,3, ST77XX_YELLOW);
        tft.fillCircle(33,27,5, ST77XX_YELLOW);
      }
      else if(type_attack == 2){
        tone(3, 831); 
        tft.fillTriangle(20,32,25,22,30,32,ST77XX_BLACK);
        tft.fillTriangle(30,32,35,22,40,32,ST77XX_BLACK);

        tft.fillTriangle(20,37,25,27,30,37,ST77XX_WHITE);
        tft.fillTriangle(30,37,35,27,40,37,ST77XX_WHITE);
      }
      else if(type_attack == 3){
        tone(3, 175);
        tft.fillCircle(20,25,5, ST77XX_BLACK);
        tft.fillCircle(22,35,8, ST77XX_BLACK);
        tft.fillCircle(30,20,4, ST77XX_BLACK);
        
        tft.fillCircle(20,25,10, ST77XX_MAGENTA);
        tft.fillCircle(22,35,8, ST77XX_MAGENTA);
        tft.fillCircle(30,20,6, ST77XX_MAGENTA); 
      }
      else if(enemy_flag == 1){
        tone(3, 659); 
        tft.invertDisplay(true);
      }
    break;
    
  }//end of state transitions for effect task
  
  return state;
}


void loop() {
  //input read
  xValue = analogRead(A0);  
  yValue = analogRead(A1);  
  bValue = digitalRead(2);
  
for (int i = 0; i < tasksNum; ++i) {
    if ( (millis() - tasks[i].elapsedTime) >= tasks[i].period) {
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = millis(); // Last time this task was ran
    }
  }//end of for loop
   delay(100);
}
