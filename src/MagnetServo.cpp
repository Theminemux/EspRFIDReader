#include "MagnetServo.h"

void MagnetServo::begin()
{
    servo.attach(SERVO_PIN);
    MoveUp();
}

void MagnetServo::MoveUp()
{
    servo.write(0);
    isUp = true;
}

void MagnetServo::MoveDown()
{
    servo.writeMicroseconds(2600);
    isUp = false;
}

bool MagnetServo::IsUp()
{
    return isUp;
}