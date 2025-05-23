/*
   Светодиодный куб 7x7x7 на Arduino NANO и сдвиговых регистрах 74HC595
*/

#define INVERT_Y 1    // инвертировать по вертикали
#define INVERT_X 0    // инвертировать по горизонтали


#define POS_X 0
#define NEG_X 1
#define POS_Z 2
#define NEG_Z 3
#define POS_Y 4
#define NEG_Y 5

#define BUTTON1 18
#define BUTTON2 19
#define RED_LED 5
#define GREEN_LED 7

#include <SPI.h>
#include "Button.h"

bool loading = true;
uint16_t timer = 0;
uint16_t modeTimer = 100;  // можно настроить под нужную скорость

GButton butt1(BUTTON1);
GButton butt2(BUTTON2);

uint8_t cube[7][7];  // 7x7x7 куб

void setup() {
  Serial.begin(9600);
  
  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, HIGH);
  
  butt1.setIncrStep(5);
  butt1.setIncrTimeout(100);
  butt2.setIncrStep(-5);
  butt2.setIncrTimeout(100);
  
  clearCube();
}

void loop() {
  butt1.tick();
  butt2.tick();
  
  // Просто зажигаем весь куб
  lightCube();
  renderCube();
}
int16_t coord[3];    // координаты кубика (x,y,z)
int8_t vector[3];    // вектор движения
uint16_t walkTimer = 0;
uint16_t walkModeTimer = 100; // скорость анимации

void walkingCube() {
  if (loading) {
    clearCube();
    loading = false;
    for (byte i = 0; i < 3; i++) {
      // координаты теперь в диапазоне 0-600 (100*6)
      coord[i] = 300;
      vector[i] = random(3, 8) * 10; // уменьшил множитель для 7x7x7
    }
  }
  
  walkTimer++;
  if (walkTimer > walkModeTimer) {
    walkTimer = 0;
    clearCube();
    
    // Обновляем позицию
    for (byte i = 0; i < 3; i++) {
      coord[i] += vector[i];
      
      // Проверяем границы (теперь 600 вместо 700)
      if (coord[i] < 1) {
        coord[i] = 1;
        vector[i] = -vector[i];
        vector[i] += random(0, 6) - 3;
      }
      if (coord[i] > 600 - 100) {
        coord[i] = 600 - 100;
        vector[i] = -vector[i];
        vector[i] += random(0, 6) - 3;
      }
    }

    // Пересчитываем координаты для 7x7x7
    int8_t thisX = coord[0] / 100;
    int8_t thisY = coord[1] / 100;
    int8_t thisZ = coord[2] / 100;

    // Рисуем кубик 2x2x2 с проверкой границ
    if (thisX < 6 && thisY < 6 && thisZ < 6) {
      setVoxel(thisX, thisY, thisZ);
      setVoxel(thisX + 1, thisY, thisZ);
      setVoxel(thisX, thisY + 1, thisZ);
      setVoxel(thisX, thisY, thisZ + 1);
      setVoxel(thisX + 1, thisY + 1, thisZ);
      setVoxel(thisX, thisY + 1, thisZ + 1);
      setVoxel(thisX + 1, thisY, thisZ + 1);
      setVoxel(thisX + 1, thisY + 1, thisZ + 1);
    }
  }
}

void shift(uint8_t dir) {
  if (dir == NEG_Y) {
    for (uint8_t y = 6; y > 0; y--) {  // 6 вместо 7
      for (uint8_t z = 0; z < 7; z++) {
        cube[y][z] = cube[y-1][z];
      }
    }
    for (uint8_t i = 0; i < 7; i++) {
      cube[0][i] = 0;
    }
  }
}

void rain() {
  if (loading) {
    clearCube();
    loading = false;
  }
  timer++;
  if (timer > modeTimer) {
    timer = 0;
    shift(NEG_Y);
    uint8_t numDrops = random(0, 5);
    for (uint8_t i = 0; i < numDrops; i++) {
      setVoxel(random(0, 7), 6, random(0, 7));  // 6 вместо 7, так как у нас теперь 7 слоёв (0-6)
    }
  }
}

void renderCube() {
  for (uint8_t i = 0; i < 7; i++) {
    digitalWrite(SS, LOW);
    if (INVERT_Y) SPI.transfer(0x01 << (6 - i));  // 7 бит вместо 8
    else SPI.transfer(0x01 << i);
    
    for (uint8_t j = 0; j < 7; j++) {
      if (INVERT_X) SPI.transfer(cube[6 - i][j]);  // 7 элементов вместо 8
      else SPI.transfer(cube[i][j]);
    }
    digitalWrite(SS, HIGH);
  }
}

void setVoxel(uint8_t x, uint8_t y, uint8_t z) {
  if (x < 7 && y < 7 && z < 7) {
    cube[6 - y][6 - z] |= (0x01 << x);  // 7 вместо 8
  }
}

void clearVoxel(uint8_t x, uint8_t y, uint8_t z) {
  if (x < 7 && y < 7 && z < 7) {
    cube[6 - y][6 - z] ^= (0x01 << x);  // 7 вместо 8
  }
}

bool getVoxel(uint8_t x, uint8_t y, uint8_t z) {
  if (x < 7 && y < 7 && z < 7) {
    return (cube[6 - y][6 - z] & (0x01 << x)) == (0x01 << x);
  }
  return false;
}

void lightCube() {
  for (uint8_t i = 0; i < 7; i++) {
    for (uint8_t j = 0; j < 7; j++) {
      cube[i][j] = 0x7F;  // 7 бит вместо 8 (0x7F = 0b01111111)
    }
  }
}

void clearCube() {
  for (uint8_t i = 0; i < 7; i++) {
    for (uint8_t j = 0; j < 7; j++) {
      cube[i][j] = 0;
    }
  }
}
