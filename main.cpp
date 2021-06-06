#include "mbed.h"
#include "MMA8451Q.h"
#include "TextLCD.h"
#include "TSISensor.h"
#include "DebounceIn.h"
#include "Keypad.h"
#include "sMotor.h"

TextLCD lcd(PTE29, PTE30, PTE20, PTE21, PTE22, PTE23, TextLCD::LCD16x2);
SPI spi(PTD2, PTD3, PTD1);          
DigitalOut cs(PTD0); 
MMA8451Q ACC(PTE25, PTE24, 0x1D);
Keypad pad(PTA12, PTD4, PTA2, PTA1, PTC9, PTC8, PTA5, PTA4);
TSISensor tsi;
DebounceIn  button(PTA13);
PwmOut buzzer(PTD5);
sMotor motor(PTB11, PTB10, PTB9, PTB8);

void display_score(int distance);
int draw_catcher(int y_axis, int sensitivity);
void Init_MAX7219(void);
void draw(int *figure);
void SPI_Write2(unsigned char digit, unsigned char data);
void generate_stream(char line, int difficulty, int sensitivity, int speed);
void clear_dot(void);
int swipe(void);
void floating_dot(int sensitivity, int *mem);
void add_dot(int *memory, int row, int col);
void delete_dot(int *memory, int row, int col);
char read_char(void);
int take_iput(void);
void jump(void);
int calculate_score(int *memory, int *figure);
int take_input(void);

unsigned char random_lines[] = 
    {2, 1, 5, 5, 3, 8, 4, 7, 5, 3, 6, 8,
     3, 5, 4, 6, 1, 8, 1, 6, 3, 4, 5, 2};
                        

int state = 0;

int main(void)
{
    motor.step(5, 1, 800);
    Init_MAX7219();
    buzzer.period(0.005);
    
    int swipe_or_jump;
    
menu:

    while(button.read() == 1)
    {
        lcd.cls();
        
        if (state == 0)
        {
            lcd.printf(" WELCOME TO THE ");
            lcd.locate(0, 1);
            lcd.printf("    DOTGAME   ");
            swipe_or_jump = swipe();
            wait(1);
        }
        
        else if (state == 1)
        {
            lcd.printf("Catch The Stream");
            lcd.locate(0, 1);
            lcd.printf("Press To Start");
            swipe_or_jump = swipe();
            if (swipe_or_jump == 1)
            {
                goto startCatch;
            }
            wait(1);
        }
        
        else if (state == -1)
        {
            lcd.printf("DrawMe If U CAN");
            lcd.locate(0, 1);
            lcd.printf("Press To Start");
            swipe_or_jump = swipe();
            if (swipe_or_jump == 1)
            {
                goto start;
            }
            wait(1);
        }
    }
    
start:
    
    lcd.cls();
    lcd.printf("Sensitivity      (1 - 9):");
    int sensitivity = 12 - take_input();
    wait(1);
    lcd.cls();
    
    lcd.printf("Easy(1)notEasy(2):");
    lcd.locate(0, 1);
    lcd.printf("Difficult(3):");
    int difficulty_level = take_input();
    wait(1);
    lcd.cls();
              
    int memory[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    int difficult[] = {0x01, 0x10, 0x01, 0x20, 0x04, 0x80, 0x40, 0x01};
    
    int easy[] = {0x81, 0x00, 0x18, 0x24, 0x24, 0x18, 0x00, 0x81};
    
    int intermediate[] = {0x81, 0x42, 0x18, 0x00, 0x00, 0x18, 0xA5, 0x00};
    
    int *figure;
    
    switch(difficulty_level)
    {
        case 1:
            figure = easy; break;
        case 2:
            figure = intermediate; break;
        case 3:
            figure = difficult; break; 
    }
    
    buzzer.write(0.5);
    wait(0.5);
    buzzer.write(0.0);
    draw(figure);
    wait(3);
    clear_dot();
    
    int row, col, y_acc, x_acc;
            
    while(1)
    {
    
        while (button.read() == 1)
        {
            y_acc = (int) (100 * ACC.getAccY());
            x_acc = (int) (100 * ACC.getAccX());
        
            if (y_acc > sensitivity * 3)
            {
                row = 8;
            }
            else if (y_acc > sensitivity * 2)
            {
                row = 7;
            }
            else if (y_acc > sensitivity * 1)
            {
                row = 6;
            }
            else if (y_acc > 0)
            {
                row = 5;
            }
            else if (y_acc > sensitivity * -1)
            {
                row = 4;
            }
            else if (y_acc > sensitivity * -2)
            {
                row = 3;
            }
            else if (y_acc > sensitivity * -3)
            {
                row = 2;
            }
            else 
            {
            row = 1;
            }
        
            if (x_acc > sensitivity * 3)
            {
                col = 1;
            }
            else if (x_acc > sensitivity * 2)
            {
                col = 2;
            }
            else if (x_acc > sensitivity * 1)
            {
                col = 3;
            }
            else if (x_acc > 0)
            {
                col = 4;
            }
            else if (x_acc > sensitivity * -1)
            {
                col = 5;
            }
            else if (x_acc > sensitivity * -2)
            {
                col = 6;
            }
            else if (x_acc > sensitivity * -3)
            {
                col = 7;
            }
            else 
            {
                col = 8;
            }
        
            
            if  (tsi.readPercentage() != 0)
            {
                goto drawn;
            }
            
            add_dot(memory, row, col);
            draw(memory);
            wait(0.15);
            delete_dot(memory, row, col);
            draw(memory);
        }
        buzzer.write(0.5);
        wait(0.05);
        buzzer.write(0.0);
        add_dot(memory, row, col);
        draw(memory);
        wait(0.5);
    }    
    
drawn:
    int score;
    score = calculate_score(memory, figure);
    lcd.cls();
    lcd.printf("Calculating...");
    wait(1);
    lcd.cls();
    lcd.printf("Score = %d", score);
    int i;
    for (i = 0; i < 5; i++)
    {
        draw(memory);
        wait(1);
        draw(figure);
        wait(1);
    }
    clear_dot();

prompt:    
    lcd.cls();
    lcd.printf("Main Menu(1)");
    lcd.locate(0, 1);
    lcd.printf("Try Again(2):");
    int choice = take_input();
    wait(1);
    lcd.cls();
    
    if (choice == 1)
    {
        goto menu; 
    }
    else if (choice == 2)
    {
        goto start;
    }
    else
    {
        lcd.printf("invalid choice!");
        wait(1);
        lcd.cls();
        goto prompt;
    }
    
    
startCatch:   

    int y;
    int line_guess;
    char random = 0;
    int current_score = 50;
    int max_score = 50;
    int missed = 0;
    
    
    lcd.cls();
    lcd.printf("Sensitivity      (1 - 9):");
    sensitivity = 12 - take_input();
    wait(1);
    lcd.cls();
    
    lcd.printf("Starting...:");
    wait(1);
    buzzer.write(0.5);
    wait(1);
    buzzer.write(0.0);
    lcd.cls();
 
    //int timer = 0;    
    while (1) 
    {
        lcd.cls();
        clear_dot();
        
        
        if (random > 15) 
            random = 0;
        else
            random++;
        
        lcd.printf("Score: ");
        display_score(current_score);
        
        if (current_score < 50)
        {
            generate_stream(random_lines[random], 2, sensitivity, 25);
        }
        else if (current_score < 100)
        {
            generate_stream(random_lines[random], 2, sensitivity, 20);
        }
        else if (current_score < 150)
        {
            generate_stream(random_lines[random], 2, sensitivity, 18);
        }
        else if (current_score < 200)
        {
            generate_stream(random_lines[random], 2, sensitivity, 16);
        }
        else if (current_score < 250)
        {
            generate_stream(random_lines[random], 2, sensitivity, 14);
        }
        else if (current_score < 300)
        {
            generate_stream(random_lines[random], 2, sensitivity, 12);
        }
        else if (current_score < 350)
        {
            generate_stream(random_lines[random], 2, sensitivity, 10);
        }
        else if (current_score < 400)
        {
            generate_stream(random_lines[random], 2, sensitivity, 8);
        }
        else if (current_score < 450)
        {
            generate_stream(random_lines[random], 2, sensitivity, 6);
        }
        else 
        {
            generate_stream(random_lines[random], 2, sensitivity, 4);
        }
        
        
        
        
        y = (int) (100 * ACC.getAccY());
        line_guess = draw_catcher(y, sensitivity);
        
        if (random_lines[random] == line_guess)
        {
            current_score = current_score + 10;
            missed = 0;
            if (current_score > max_score)
            {
                max_score = current_score;
            }
        }
        else if (current_score > 10 && missed < 3)
        {
            current_score = current_score - 20;
            missed++;
            buzzer.write(0.5);
            wait(0.03);
            buzzer.write(0.0);
        }
        else
        {
            lcd.cls();
            lcd.printf("GAME OVER");
            lcd.locate(0, 1);
            lcd.printf("Max Score: %d", max_score);
            if (max_score > 160)
            {
                motor.step(128, 1, 800);
                wait(2);
                motor.step(128, 0, 800);
            }
            else
            {
                motor.step(128, 0, 800);
                wait(2);
                motor.step(128, 1, 800);
            }
            wait(5);
            goto ask;
        }
        //timer++;
    }
    
ask:
    lcd.cls();
    lcd.printf("Main Menu(1)");
    lcd.locate(0, 1);
    lcd.printf("Try Again(2):");
    choice = take_input();
    wait(1);
    lcd.cls();
    
    if (choice == 1)
    {
        goto menu; 
    }
    else if (choice == 2)
    {
        goto startCatch;
    }
    else
    {
        lcd.printf("invalid choice!");
        wait(1);
        lcd.cls();
        goto ask;
    }
}


/*This function displays the distance on the LCD*/
void display_score(int distance)
{
    int first_digit = distance % 10;
    int second_digit = (distance / 10) % 10;
    int third_digit = distance / 100;
    if (third_digit != 0)
    {
        lcd.putc(third_digit + '0');
        lcd.putc(second_digit + '0');
    }
    else if (second_digit != 0)
    {
         lcd.putc(second_digit + '0');
    }
    lcd.putc(first_digit + '0');
} 

void SPI_Write2(unsigned char digit, unsigned char data)
{
    cs = 0;                         
    spi.write(digit);               //address of digit reg
    spi.write(data);                // LED sequence
    cs = 1;                         
}



/// MAX7219 initialisation
void Init_MAX7219(void)
{
    cs = 1;
    SPI_Write2(0x09, 0x00);         // Decoding off
    SPI_Write2(0x0A, 0x08);         // Brightness to intermediate
    SPI_Write2(0x0B, 0x07);         // Scan limit = 7
    SPI_Write2(0x0C, 0x01);         // Normal operation mode
    SPI_Write2(0x0F, 0x0F);         // Enable display test
    wait_ms(500);                   // 500 ms delay
    SPI_Write2(0x01, 0x00);         // Clear row 0.
    SPI_Write2(0x02, 0x00);         // Clear row 1.
    SPI_Write2(0x03, 0x00);         // Clear row 2.
    SPI_Write2(0x04, 0x00);         // Clear row 3.
    SPI_Write2(0x05, 0x00);         // Clear row 4.
    SPI_Write2(0x06, 0x00);         // Clear row 5.
    SPI_Write2(0x07, 0x00);         // Clear row 6.
    SPI_Write2(0x08, 0x00);         // Clear row 7.
    SPI_Write2(0x0F, 0x00);         // Disable display test
    wait_ms(500);                   // 500 ms delay
}

int draw_catcher(int y_axis, int sensitivity)
{
    if (y_axis > sensitivity * 4)
    {
        SPI_Write2(1, 0x00);
        return -1;
    }
    else if (y_axis > sensitivity * 3)
    {
        SPI_Write2(1, 0x01);
        return 1;
    }
    else if (y_axis > sensitivity * 2)
    {
        SPI_Write2(1, 0x02);
        return 2;
    }
    else if (y_axis > sensitivity * 1)
    {
        SPI_Write2(1, 0x04);
        return 3;
    }
    else if (y_axis > 0)
    {
        SPI_Write2(1, 0x08);
        return 4;
    }
    else if (y_axis > sensitivity * -1)
    {
        SPI_Write2(1, 0x10);
        return 5;
    }
    else if (y_axis > sensitivity * -2)
    {
        SPI_Write2(1, 0x20);
        return 6;
    }
    else if (y_axis > sensitivity * -3)
    {
        SPI_Write2(1, 0x40);
        return 7;
    }
    else if (y_axis > sensitivity * -4)
    {
        SPI_Write2(1, 0x80);
        return 8;
    }    
    else 
    {
        SPI_Write2(1, 0x00);
        return -1;
    }            
}


void generate_stream(char line, int difficulty, int sensitivity, int speed)
{
    int data;
    float duration = speed / 100.0;
    int y_acc;
    switch (line)
    {
        case 1:
        data = 0x01; break;
        case 2:
        data = 0x02; break;
        case 3:
        data = 0x04; break;
        case 4:
        data = 0x08; break;
        case 5:
        data = 0x10; break;
        case 6:
        data = 0x20; break;
        case 7:
        data = 0x40; break;
        case 8:
        data = 0x80; break;
    }
    
    int i;
    for (i = 8; i > 1; i--)
    {
        SPI_Write2(i, data);
        if (difficulty == 2)
        {
            y_acc = (int) (100 * ACC.getAccY());
            draw_catcher(y_acc, sensitivity);
        }
        wait(duration);
    }
}

void clear_dot(void)
{
    int i;
    for (i = 8; i > 0; i--)
    {
        SPI_Write2(i, 0x00);
    }
}


int swipe(void)
{
    int tsi_1, tsi_2;
    while(true)
    {
        if (tsi.readPercentage() != 0)
        {
            tsi_1 = (int) (100 * tsi.readPercentage());
            wait(0.3);
            tsi_2 = (int) (100 * tsi.readPercentage());
            
            if (tsi_1 > tsi_2 && state > -1)
            {
                state--;
            }
            else if (tsi_1 < tsi_2 && state < 1)
            {
                state++;
            }
            return 0;
        }
        if (button.read() == 0)
        {
            return 1;
        }
    } 
}



void floating_dot(int sensitivity, int *mem)
{
    int row, col, y_acc, x_acc;
    while (button.read() == 1)
    {
        y_acc = (int) (100 * ACC.getAccY());
        x_acc = (int) (100 * ACC.getAccX());
        
        if (y_acc > sensitivity * 3)
        {
            row = 8;
        }
        else if (y_acc > sensitivity * 2)
        {
            row = 7;
        }
        else if (y_acc > sensitivity * 1)
        {
            row = 6;
        }
        else if (y_acc > 0)
        {
            row = 5;
        }
        else if (y_acc > sensitivity * -1)
        {
            row = 4;
        }
        else if (y_acc > sensitivity * -2)
        {
            row = 3;
        }
        else if (y_acc > sensitivity * -3)
        {
            row = 2;
        }
        else 
        {
            row = 1;
        }
        
        if (x_acc > sensitivity * 3)
        {
            col = 1;
        }
        else if (x_acc > sensitivity * 2)
        {
            col = 2;
        }
        else if (x_acc > sensitivity * 1)
        {
            col = 3;
        }
        else if (x_acc > 0)
        {
            col = 4;
        }
        else if (x_acc > sensitivity * -1)
        {
            col = 5;
        }
        else if (x_acc > sensitivity * -2)
        {
            col = 6;
        }
        else if (x_acc > sensitivity * -3)
        {
            col = 7;
        }
        else 
        {
            col = 8;
        }
        
        add_dot(mem, row, col);
        wait(0.1);
        delete_dot(mem, row, col);
    }
    add_dot(mem, row, col);
}


void add_dot(int *memory, int row, int col)
{
    int data;
    col = 9 - col;
    switch (row)
    {
        case 8:
        data = 0x01; break;
        case 7:
        data = 0x02; break;
        case 6:
        data = 0x04; break;
        case 5:
        data = 0x08; break;
        case 4:
        data = 0x10; break;
        case 3:
        data = 0x20; break;
        case 2:
        data = 0x40; break;
        case 1:
        data = 0x80; break;
    }
    
    int store = memory[col - 1];
    int bitStatus = (store & data) >>  (8 - row);

    
    if (bitStatus != 1)
    {
        memory[col - 1] = memory[col - 1] + data;
    }
}

void draw(int *figure)
{
    int i;
    for (i = 0; i < 8; i++)
    {
         SPI_Write2(i + 1, figure[i]);   
    }  
}


void delete_dot(int *memory, int row, int col)
{
    int data;
    col = 9 - col;
    switch (row)
    {
        case 8:
        data = 0x01; break;
        case 7:
        data = 0x02; break;
        case 6:
        data = 0x04; break;
        case 5:
        data = 0x08; break;
        case 4:
        data = 0x10; break;
        case 3:
        data = 0x20; break;
        case 2:
        data = 0x40; break;
        case 1:
        data = 0x80; break;
    }
    
    int store = memory[col - 1];
    int bitStatus = (store & data) >>  (8 - row);

    
    if (bitStatus != 0)
    {
        memory[col - 1] = memory[col - 1] - data;
    }
}

int take_input(void)
{
    int input;
    
    int key1 = read_char();
    wait(0.5);
    lcd.printf("%c", key1);
    
    input = key1 - 48;
    return input;
}


/*This function waits for a key and 
returns the ASCII when a key pressed*/
char read_char(void)
{
    int released = 1;
    char key;
    while(1)
    {
        key = pad.ReadKey();               

        if(key == '\0')
            released = 1;                       

        if((key != '\0') && (released == 1)) 
            return key;            
    }   
}

int calculate_score(int *memory, int *figure)
{
    
    int row, col;
    int data;    
    int check1, check2;
    int memory_col, figure_col;
    int score = 0;
    
    
    for (row = 1; row < 9; row++)
    {
        switch (row)
        {
            case 8:
                data = 0x01; break;
            case 7:
                data = 0x02; break;
            case 6:
                data = 0x04; break;
            case 5:
                data = 0x08; break;
            case 4:
                data = 0x10; break;
            case 3:
                data = 0x20; break;
            case 2:
                data = 0x40; break;
            case 1:
                data = 0x80; break;
        }
        for (col = 1; col < 9; col++)
        {   
            memory_col = memory[col - 1];
            figure_col = figure[col - 1];  
                  
            check1 = (memory_col & data) >> (8 - row);
            check2 = (figure_col & data) >> (8 - row);
            if (check1 == 1 && check2 == 1)
            {
                score += 10;
            }
            else if (check2 == 0 && check1 == 1)
            {
                score -= 10;
            }
        }
    } 
    return score;        
}
