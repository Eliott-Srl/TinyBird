#include <Tiny4kOLED.h>
#include <TinyWireM.h>
#include <MPU6050_tockn.h>

#define TAILLE_BITMAP 4*8
#define BUZZER 1
MPU6050 mpu6050(Wire);

const int screen_width = 128; // Largeur de l'écran
const int screen_height = 32; // Hauteur de l'écran
const int nbVariant = 4;
int score = 0;
int flappy_y = screen_height / 2;

const int bouton_droite = A2;
const int bouton_gauche = A3;
const int max = (2*2*2*2*2) - 1;

const int boutons[4] = {20, 18, 15, 22};
// l'indice du bouton   0   1   2   3

int first = 0;

unsigned long m_w = 1;
unsigned long m_z = 2;

const uint8_t s_h_m_b [TAILLE_BITMAP] PROGMEM = {
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

const uint8_t m_h_s_b [TAILLE_BITMAP] PROGMEM = {
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

const uint8_t b_b [TAILLE_BITMAP] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

const uint8_t b_h [TAILLE_BITMAP] PROGMEM = {
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

struct tube {
  int x;
  const uint8_t *bitmap;
};

tube blocks[2];

unsigned long getRandom() {
    m_z = 36969L * (m_z & 65535L) + (m_z >> 16);
    m_w = 18000L * (m_w & 65535L) + (m_w >> 16);
    return (m_z << 16) + m_w;  /* 32-bit result */
}

// Si bouton vaut 0 alors on regarde les boutons de gauche et de droite pour le contraire
int boutonAppuyer(int bouton) {
  int etat_bouton = 0;
  int etat_bouton_simple = 0;

  if (!bouton) {
    etat_bouton = map(analogRead(bouton_droite), 0, 1023, 0, max);
  } else {
    etat_bouton = map(analogRead(bouton_gauche), 0, 1023, 0, max);
  }

  etat_bouton_simple = ((etat_bouton) &= ~(1UL << (5))); // The implementation of bitClear

  for (int j = 0; j < 4; j++) {
    if (boutons[j] == etat_bouton_simple) {
        return j + 1;
    }
  }

  return 0;
}


const uint8_t *bitmapRandom() {
  int bitmapTexture = getRandom() % nbVariant;

  if (bitmapTexture == 0) {
    return s_h_m_b;
  } else if (bitmapTexture == 1) {
     return m_h_s_b;
  } else if (bitmapTexture == 2) {
     return b_b;
  } else {
     return b_h;
  }
}

void newRandomBlock() {
  int idx = 0;
  if (first == 0) {
    idx = 1;
  }

  blocks[idx].x = screen_width;
  blocks[idx].bitmap = bitmapRandom();
}

void incrementePosX() {
  for (int i = 0; i < 2; i++) {
    if (blocks[i].x > -1) {
      blocks[i].x -= 2;
    }
  }

  if (blocks[first].x < 0) {
    blocks[first].x = -1;
    score++;
    PORTB |= (1 << BUZZER);
    delay(100);
    PORTB &= ~(1 << BUZZER);
    if (first == 0) {
      first = 1;
    } else {
      first = 0;
    }
  }
}

void verifyBlocks() {
  int idx = 0;
  if (first == 0) {
    idx = 1;
  }

  if (blocks[first].x < screen_width / 2 && blocks[idx].x < 0) {
    newRandomBlock();
  }
}

void afficherBlocks() {
  for (int i = 0; i < 2; i++) {
    if (blocks[i].x > -1) {
      oled.bitmap(blocks[i].x, 0, blocks[i].x + 8, 4, m_h_s_b);
    }
  }
}

int detecterCollisions() {
  for (int i = 0; i < 2; i++) {
    if (screen_width/4 > blocks[i].x && screen_width/4 < blocks[i].x + 8 && (int) (blocks[i].bitmap[flappy_y * 8]) != 0x00) {
      return 1;
    }
  }
  return 0;
}

void initialiseBlocks() {
  first = 0;
  blocks[0].x = screen_width;
  blocks[0].bitmap = bitmapRandom();
  blocks[1].x = -1;
  blocks[1].bitmap = bitmapRandom();
}

void setup() {
  //initialisation
  TinyWireM.begin();
  mpu6050.begin();
  // mpu6050.calcGyroOffsets();
  oled.begin();
  oled.setFont(FONT6X8); // Utiliser une police plus petite pour dessiner un point unique
  initialiseBlocks();
  oled.clear();
  oled.on();
}

void loop() {
  //Actualisation du MPU
  oled.clear();
  oled.setCursor(0, 0);
  oled.print(F("Bleu: Jouer\nJaune: Regles"));
  oled.on();

  int menu = boutonAppuyer(0);
  while (!menu) {
    menu = boutonAppuyer(0);
  }

  if (menu == 4) {
    int quitter = 0;
    initialiseBlocks();
    while (quitter == 0) {
      mpu6050.update();
      incrementePosX();
      verifyBlocks();

      if (boutonAppuyer(1) == 1) {
        flappy_y++;
      }

      if (boutonAppuyer(1) == 3) {
        flappy_y--;
      }
      // Lecture des valeurs gyroscopiques
      // recuperation des angles
      int angle = (mpu6050.getAngleY() - 40) / -100;
      flappy_y += angle;

      oled.clear();
      oled.setCursor(screen_width/4, flappy_y);
      oled.print(F("o"));
      oled.setCursor(0, 0);


      if (detecterCollisions()) {
        quitter = 3;
      }

      quitter = boutonAppuyer(0);

      afficherBlocks();
      oled.on();
    }
    menu = 0;
  } else if (menu == 2) {
    oled.clear();

    oled.setCursor(0, 0);
    oled.print(F("Regle: Des tuyaux viennent de droite et il faut les éviter !!\nRouge: Quitter"));
    oled.on();

    int quitter = 0;
    while (!quitter) {
      quitter = boutonAppuyer(0);
    }
    menu = 0;
  }
}
  