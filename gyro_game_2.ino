#include <Arduino.h>

#include <SPI.h>
#include <SD.h>        // SD-Karte (funktioniert vlt iwann mal?!)

#include <Adafruit_GFX.h>
#include <Waveshare4InchTftShield.h>

#define error(s) sd.errorHalt(F(s))  // store error strings in flash to save RAM

#define BLACK   0x0000  // Farben zum leichteren Zugriff
#define BLUE    0x001F
#define GREEN   0x07E0

#define MAX_ENEMIES 40  // maximale Anzahl roter Punkte

Waveshare4InchTftShield Waveshield; // Waveshield-TFT erstellen
Adafruit_GFX &tft = Waveshield;     // und mit Adafruit-Lib koppeln



bool SDCard=true;  // Kann die Highscore gespeichert werden?

int timeLeft = 90,             // verbleibende Spielzeit
    timeLeftOld = 0;           // testet, ob sich was getan hat

byte gameSpeed = 40;           // Spielgeschwindigkeit (nicht mehr als 50!)

int ax, ay, az,                // Achsen des Gyrosensors
    calibX, calibY, calibZ;    // Kalibrierung

    
int bkColor = BLUE;                 // Hintergrundfarbe des Spielfelds festlegen

// Ja, man könnte das Ganze wirklich objektorientierter gestalten...
// Mach ich vielleicht auch iwann.

// Spielerdaten
int playerX = 20, playerY = 20, playerRadius = 10,
    playerSpeedX = 1, playerSpeedY = 1, playerSpeedZ = 1; 
    
int playerColor = GREEN,
    playerXold, playerYold,
    playerScore = 0, playerScoreOld = 1;

// Bonusdaten
int bonusX, bonusY, 
    bonusXold, bonusYold;
unsigned int bonusRadius = 10, bonusColor = GREEN;
    
int numEnemies=-1; // aktuelle ANzahl der roten Punkte

int enemyX[MAX_ENEMIES], enemyY[MAX_ENEMIES],
    enemyXold[MAX_ENEMIES], enemyYold[MAX_ENEMIES];
unsigned int enemyRadius[MAX_ENEMIES], enemyColor[MAX_ENEMIES];

char highscoreName[11][4],
     highscorePoints[11][4];
File fHighscore;                // Datei für die Highscore
char charName[4] = {'M','J','K','\0'}; // aktuell eingegebener Highscore-Name

unsigned int timeToInit;        //Zeit, die für die Initialisierung benötigt wird



///////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t rgb2int(uint8_t r, uint8_t g, uint8_t b) // Umwandlung von GRB-Wert in Interger
{ 
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int addEnemy()
{
  numEnemies++;
  newEnemy(numEnemies);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int removeEnemy()
{
  tft.fillCircle(enemyX[numEnemies], enemyY[numEnemies], enemyRadius[numEnemies], bkColor);
  numEnemies--;
  for (int i = 0; i < numEnemies - 1; i++) drawEnemy(i, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int newBonus()
{
  bool bonusCollides = false; 
  bonusRadius = 10;
   
  do {
    bonusX = random(tft.width() - bonusRadius) + bonusRadius;
    bonusY = random(tft.height() - bonusRadius - 20) + bonusRadius + 20;
    bonusCollides = false;
    for (int i=0; i < numEnemies + 1; i++)
    {  
      bonusCollides = (((bonusX > playerX - playerRadius - 10) && (bonusX < playerX + playerRadius + 10) && (bonusY > playerY - playerRadius - 10) && (bonusY < playerX + playerRadius + 10)) ||
                       ((enemyX[i] > bonusX - bonusRadius - 10) && (enemyX[i] < bonusX + bonusRadius + 10) && (enemyY[i] > bonusY - bonusRadius - 10) && (enemyY[i] < bonusY + bonusRadius + 10)));
      if (bonusCollides) break;
    }
  } 
  while (bonusCollides == true);
  
  drawBonus(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int newEnemy(int num)
{
  enemyRadius[num]=5;
  enemyColor[num]=GREEN;
  // der rote Punkt soll nicht beim Bonus und nicht beim Spieler landen. Das wäre echt unfair ;-)
  do 
  {     
    enemyX[num] = random(tft.width() - enemyRadius[num]) + enemyRadius[num];
    enemyY[num] = random(tft.height() - enemyRadius[num] - 20) + enemyRadius[num] + 20;
  }
  while (((enemyX[num] > playerX - playerRadius - 10) && (enemyX[num] < playerX + playerRadius + 10) && (enemyY[num] > playerY - playerRadius - 10) && (enemyY[num] < playerY + playerRadius + 10)) ||
         ((enemyX[num] > bonusX - bonusRadius - 10) && (enemyX[num] < bonusX + bonusRadius + 10) && (enemyY[num] > bonusY - bonusRadius - 10) && (enemyY[num] < bonusY + bonusRadius + 10)));
 
  drawEnemy(num, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int bonusCollision()
{
  if ((playerX + playerRadius > bonusX - bonusRadius) &&
      (playerY + playerRadius > bonusY - bonusRadius) &&
      (playerX - playerRadius < bonusX + bonusRadius) &&
      (playerY - playerRadius < bonusY + bonusRadius)) {
    newBonus();
    drawPlayer(1);
    for (int i = 0; i < numEnemies + 1; i++) drawEnemy(i, 0);
    playerScore += 10;
    addEnemy();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int enemyCollision(int num)
{
  if ((playerX + playerRadius > enemyX[num] - enemyRadius[num]) &&
      (playerY + playerRadius > enemyY[num] - enemyRadius[num]) &&
      (playerX - playerRadius < enemyX[num] + enemyRadius[num]) &&
      (playerY - playerRadius < enemyY[num] + enemyRadius[num])) {
    newEnemy(num);
    removeEnemy();
    for (int i = 0; i < numEnemies + 1; i++) drawEnemy(i, 0);
    playerScore -= 20;
    if (playerScore < 0) playerScore = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int drawPlayer(int force)
{
  if ((playerX != playerXold) || (playerY != playerYold) || (force == 1))
  {
    tft.fillCircle(playerXold, playerYold, playerRadius, bkColor);
    tft.fillCircle(playerX, playerY, playerRadius, playerColor);
    playerXold = playerX; playerYold = playerY;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int drawBonus(int force)
{
  if ((force == 1) || (bonusX != bonusXold) || (bonusY != bonusYold)) {
    tft.fillCircle(bonusXold, bonusYold, bonusRadius, bkColor);
    tft.fillCircle(bonusX, bonusY, bonusRadius, bonusColor);
    bonusXold = bonusX; bonusYold = bonusY;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int drawEnemy(int num, int force)
{
  if ((enemyX[num] != enemyXold[num]) || (enemyY[num] != enemyYold[num]) || (force == 1)) {
    tft.fillCircle(enemyXold[num], enemyYold[num], enemyRadius[num], bkColor);
    tft.fillCircle(enemyX[num], enemyY[num], enemyRadius[num], rgb2int(255, 0, 0));
    enemyXold[num] = enemyX[num]; enemyYold[num] = enemyY[num];
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int readHighscoreFromFile()
{
  char linebuf[11]; // eingelesene Zeile zwischenspeichern
  int counter=0;

  fHighscore=SD.open("score.txt", FILE_READ); // Datei öffnen

  for (int i=0; i<10; i++) {
    memset(linebuf,'0',sizeof(linebuf)); // ausnullen des Zeilenspeichers
    counter=0;
    while (fHighscore.available()) {
      linebuf[counter]=fHighscore.read();
      if (linebuf[counter]=='\n') break; // Zeilenende, Schleife verlassen
      if (counter<sizeof(linebuf)-1) counter++;
    }
    highscoreName[i][0]=linebuf[0];
    highscoreName[i][1]=linebuf[1];
    highscoreName[i][2]=linebuf[2];
    highscoreName[i][3]='\0';
    highscorePoints[i][0]=linebuf[4];
    highscorePoints[i][1]=linebuf[5];
    highscorePoints[i][2]=linebuf[6];
    highscorePoints[i][3]='\0';
  }
  
  // Einen elften Highscore-Eintrag mit den aktuellen Punkten erstellen
  char savePoints[4];
  char* sP;
  highscoreName[10][0] = charName[0];  
  highscoreName[10][1] = charName[1];
  highscoreName[10][2] = charName[2];
  highscoreName[10][3] = '\0';
  sP = itoa(playerScore, savePoints, 10);
  if (playerScore<100) savePoints[0]='0';  // evtl führende Nullen hinzufügen
  if (playerScore<10) savePoints[1]='0';
  highscorePoints[10][0] = savePoints[0];
  highscorePoints[10][1] = savePoints[1];
  highscorePoints[10][2] = savePoints[2];
  highscorePoints[10][3] = '\0';
 
  fHighscore.close();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int writeHighscoreToFile()
{
  fHighscore=SD.open("score.txt", FILE_WRITE);  // Datei öffnen
 
  // Jetzt wird sortiert!
  for(unsigned int i = 0; i < 10; i++)  // Alle Elemente durchgehen (letztes ausgenommen)
  {
    unsigned int min_pos = i;  // Position des zurzeit kleinstes Elementes
   
    // unsortierten Teil des Feldes durchlaufen
    // und nach kleinstem Element suchen
    for (unsigned int j = i + 1; j < 11; j++)
      if (atoi(highscorePoints[j]) > atoi(highscorePoints[min_pos])) min_pos = j;
    
    // Elemente vertauschen:
    // Das kleinste Element kommt an das Ende
    // bereits sortierten Teils des Feldes
    char temp[4];   //Punkte zwischenspeichern
    char temp2[4];  // Namen zwischenspeichern
    temp[0] = highscorePoints[i][0];
    temp[1] = highscorePoints[i][1];
    temp[2] = highscorePoints[i][2];
    temp[3] = '\0';
    temp2[0] = highscoreName[i][0];    
    temp2[1] = highscoreName[i][1];    
    temp2[2] = highscoreName[i][2];    
    temp2[3] = '\0';    
    highscorePoints[i][0] = highscorePoints[min_pos][0]; // Punkte tauschen
    highscorePoints[i][1] = highscorePoints[min_pos][1]; 
    highscorePoints[i][2] = highscorePoints[min_pos][2]; 
    highscorePoints[min_pos][0] = temp[0];
    highscorePoints[min_pos][1] = temp[1];
    highscorePoints[min_pos][2] = temp[2];
    highscoreName[i][0] = highscoreName[min_pos][0];  // auch den Namen tauschen, sonst werden NUR die Punkte vertauscht ...
    highscoreName[i][1] = highscoreName[min_pos][1];  // ... und das führt zu falschen Lorbeeren ;-)
    highscoreName[i][2] = highscoreName[min_pos][2];  
    highscoreName[min_pos][0] = temp2[0];             
    highscoreName[min_pos][1] = temp2[1];            
    highscoreName[min_pos][2] = temp2[2];            
  }

  // alles (sortiert) wieder in die Datei schreiben
  for (unsigned int i = 0; i < 11; i++)
  {
    fHighscore.print(highscoreName[i][0]);
    fHighscore.print(highscoreName[i][1]);
    fHighscore.print(highscoreName[i][2]);
    fHighscore.print(" ");
    fHighscore.print(highscorePoints[i][0]);
    fHighscore.print(highscorePoints[i][1]);
    fHighscore.println(highscorePoints[i][2]);
  }
  fHighscore.close();  // Datei wieder schließen
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int processHighscore()  // Namen eingeben, Highscore laden, aktualisieren und anzeigen
{
  TSPoint p;

  if (!SDCard) { // keine SD-Karte?
    tft.fillRect(0, 0, 480, 320, BLACK);
    tft.setTextColor(rgb2int(255, 0, 0));
    tft.print(F("Keine SD-Karte gefunden, Highscore nicht möglich :-("));
    do { /*nix*/ } while (333==333);
  }

  tft.fillRect(0,0,480,320,BLUE);
  tft.fillRoundRect(40, 160,40,80,10,rgb2int(0,255,0));
  tft.fillRoundRect(160,160,40,80,10,rgb2int(0,255,0));
  tft.fillRoundRect(280,160,40,80,10,rgb2int(0,255,0));
  tft.fillRoundRect(380,180,80,40,10,rgb2int(0,255,0));
  tft.fillTriangle(60, 100,40, 140,80, 140,rgb2int(0,255,0));
  tft.fillTriangle(180,100,160,140,200,140,rgb2int(0,255,0));
  tft.fillTriangle(300,100,280,140,320,140,rgb2int(0,255,0));
  tft.fillTriangle(60, 300,80, 260,40, 260,rgb2int(0,255,0));
  tft.fillTriangle(180,300,200,260,160,260,rgb2int(0,255,0));
  tft.fillTriangle(300,300,280,260,320,260,rgb2int(0,255,0));
  tft.setTextSize(2); tft.setTextColor(BLACK); tft.setCursor(385,190); tft.print(F("fertig"));
  tft.setTextColor(rgb2int(0,255,0));tft.setCursor(120,40); tft.print(F("Deine Punktzahl: ")); tft.print(playerScore);
  tft.setTextSize(3); tft.setTextColor(BLACK);
  tft.setCursor(50, 190); tft.print(charName[0]);
  tft.setCursor(170,190); tft.print(charName[1]);
  tft.setCursor(290,190); tft.print(charName[2]);
  
  do
  {
    p = Waveshield.getPoint();      // Rohdaten vom Touchscreen holen ...
    Waveshield.normalizeTsPoint(p); // ... und in Bildschirmkoordinaten umwandeln

    if (p.z>10) {   // Wurde der Touchscreen gedrückt?
      // Die ganzen Eingabefelder abfragen
      // (Ja, ja... mit Objekten wäre besser ;-)
      if (p.x>20 && p.x<100 && p.y>80 && p.y<180) charName[0]--;
      if (p.x>140 && p.x<220 && p.y>80 && p.y<180) charName[1]--;
      if (p.x>260 && p.x<340 && p.y>80 && p.y<180) charName[2]--;
      if (p.x>20 && p.x<100 && p.y>220 && p.y<320) charName[0]++;
      if (p.x>140 && p.x<220 && p.y>220 && p.y<320) charName[1]++;
      if (p.x>260 && p.x<340 && p.y>220 && p.y<320) charName[2]++;

      // Die Buchstaben in "menschlichem Rahmen" halten
      for (int i=0; i<3; i++) {
        if (charName[i]<65) charName[i]=90;
        if (charName[i]>90) charName[i]=65;
      }

      // wunderschön zeichnen
      tft.fillRoundRect(40, 160,40,80,10,rgb2int(0,255,0));
      tft.fillRoundRect(160,160,40,80,10,rgb2int(0,255,0));
      tft.fillRoundRect(280,160,40,80,10,rgb2int(0,255,0));
      tft.setCursor(50, 190); tft.print(charName[0]);
      tft.setCursor(170,190); tft.print(charName[1]);
      tft.setCursor(290,190); tft.print(charName[2]);
      // die nächste Zeile würde dafür sorgen, dass gar nichts mehr passiert
      //while (p.z>10) {} // solange nix machen, bis der "Touch" aufgehört hat
      delay(100); // ...stattdessen einfach kurz warten, damit die Buchstabenauswahl nicht durchdreht
    }
  } while (!(p.x>380 && p.y>180 && p.x<480 && p.y<220));  // das Ganze solange wiederholen, bis auf "fertig" gedrückt wurde

  readHighscoreFromFile();  // die Highscore in den Speicher laden
  SD.remove("score.txt");   // und von der SD-Karte löschen! Sonst werden alle Daten angehängt und die Datei wird riesig. Ich brauche doch nur eine "Top-Ten"
  writeHighscoreToFile();   // Werte sortieren (inkl. aktuellen Punkten) und in die Datei schreiben

  // Highscore anzeigen
  tft.fillRect(0, 0, 480, 320, BLACK);
  tft.setTextColor(rgb2int(0, 255, 0)); tft.setCursor(0, 0);
  tft.print(F("\n  -][  BESTENLISTE:  ][-\n\n")); tft.setTextSize(2);
  for (unsigned int i = 0; i < 10; i++) {
    if (highscoreName[i]==charName) {
      tft.setTextColor(rgb2int(255,0,0)); tft.setTextSize(3);
    } else {
      tft.setTextColor(rgb2int(0,255,0)); tft.setTextSize(2);
    }
    tft.print(F("             \t ")); tft.print(highscoreName[i]); tft.print(F(" \t  ")); tft.println(highscorePoints[i]);  
  }
  tft.setTextSize(3); tft.print(F("\n  druecke RESET fuer ein\nneues Spiel,wenn du magst!"));
  do { /*nichts*/ } while (333 == 333); // Für immer und ewig warten (Bei meinem Spiel ist der Resetknopf für "neues Spiel" zuständig!)
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  randomSeed(analogRead(A1)); // dem Zufall etwas auf die Sprünge helfen
  // LCD-Display initialisieren
  SPI.begin();
  Waveshield.begin();
  tft.setRotation(1);

  Serial.begin(9600);
  
  //analogReference(EXTERNAL); // seitdem ich den tollen Akku meiner aufgebohrten Ex-Powerbank benutze, hab ich keine Spannungsschwankungen mehr!


  tft.setTextSize(2);
  tft.print(F("Bitte das Spiel gerade halten,\nich kalibriere...\n"));
  delay(1000);
    calibY = analogRead(A3); // Werte der Achsen auslesen, während das Board gerade gehalten wird,
    calibX = analogRead(A4); // um so eine Kallibrierung durchzuführen
    calibZ = analogRead(A5);
  tft.print(F("fertig.\n"));
  

  if (!  /*card.init(SPI_HALF_SPEED, 5)*/ SD.begin(5)) // SD-Karte initialisieren
  {
    tft.print(F("SD-Karte nicht erkannt. Keine Highscore :-(\n"));
    SDCard=false;
    delay(2000);
  } else {
    tft.print(F("SD-Karte in Ordnung :-)\n"));
    delay(500);
    SDCard=true;
  }
  
  tft.fillRect(0, 0, tft.width(), tft.height(), bkColor);   //Spielfeld zeichenen
  tft.drawFastHLine(0, 19, tft.width(), rgb2int(0, 255, 0));  // obere Spielfeldbegrenzung zeichnen
  // Beschriftung
  tft.setTextSize(2); tft.setTextColor(rgb2int(0, 255, 255)); 
  tft.setCursor(200, 3); tft.print(F("deine Punkte: "));
  tft.setCursor(0, 3); tft.print(F("Zeit:"));

  playerY = tft.height() / 2; // Spieler vertikal ausrichten

  newBonus();    // ersten Bonus erstellen...
  drawBonus(1);  // ...und zeichnen
  playerColor=rgb2int(0, 255, 0); // Spielerfarbe einstellen
  drawPlayer(1); // Spieler zeichnen

  timeToInit=millis(); // Initialisierung fertig; Zeit nehmen, damit später nicht zu wenig Spielzeit bleibt
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
  ay = analogRead(A3) - calibY;  // Daten der drei Achsen auslesen und die Werte abziehen, die entstanden, als das Teil gerade gehalten wurde
  ax = analogRead(A4) - calibX;
  az = analogRead(A5) - calibZ;  // Z könnte man irgendwann zum "hüpfen" benutzen...
    
  playerSpeedX = (ay / 3);       // Geschwindigkeit spielbar machen
  playerSpeedY = (ax / 3);
  playerSpeedZ = az / 3;
    
    
  drawPlayer(0);     //Spielball zeichnen
  bonusCollision();  //Kollision mit Bonus abfragen
  
  //Kollision für jeden roten Ball abfragen
  for (int i = 0; i < numEnemies + 1; i++)
    enemyCollision(i); 
 
  playerXold = playerX; playerYold = playerY; //alte Spielerposition speichern für Neuzeichnen

  // Spieler je nach Neigung des Sensors bewegen und innerhalb der Spielfeldgrenzen festhalten
  playerX += playerSpeedX;
  if (playerX < playerRadius) playerX = playerRadius;
  if (playerX > tft.width() - playerRadius) playerX = tft.width() - playerRadius;
  playerY += playerSpeedY;
  if (playerY < playerRadius + 20) playerY = playerRadius + 20;  //+20 wegen Leiste oben
  if (playerY > tft.height() - playerRadius) playerY = tft.height() - playerRadius;
    
    
  timeLeft = 60 - (millis()- timeToInit) / 1000;  // Die Zeit läuft!
    
  if (timeLeft != timeLeftOld) // Zeug nur anzeigen bei veränderten Werten 
  {            
    tft.fillRect(60, 0, 71 ,18, bkColor);
    if (timeLeft > 10) tft.setTextColor(rgb2int(0, 255, 0), bkColor);
      else tft.setTextColor(rgb2int(255, 0, 0), bkColor);
    tft.setCursor(60, 3);
    tft.print(timeLeft);
    timeLeftOld = timeLeft;
  }

  if (playerScore != playerScoreOld) // Zeug nur anzeigen bei veränderten Werten
  {                                  // to do: auslagern in drawScore(), alte Funktion weg
    tft.fillRect(360, 0, 400 ,18, bkColor);
    tft.setCursor(360, 3);
    tft.setTextColor(rgb2int(0, 255, 0));
    tft.print(playerScore);
    playerScoreOld = playerScore;
  }

  delay(50 - gameSpeed); // to do: steuern der Geschwindigkeit mit Millis() und eigener Delay-Funktion
             // aber in diesem Fall egal, der Prozessor wird auf ewig der gleiche sein!
    
  if (timeLeft <= 0) processHighscore(); // Highscore: Namen eingeben, lesen, schreiben, anzeigen
}
