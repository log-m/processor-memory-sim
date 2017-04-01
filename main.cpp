//Logan Morris
//lhm140030
//CS 4348.001
//Project 1

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <regex>
#include <string>
using namespace std;

//**DECLARATION BLOCK
int getRandom(int);
int addReg(int, int);
int subReg(int, int);
int copyTo(int, int);
int copyFrom(int, int);
void putPort(int, int);
void kernelMode(int[], int[], int[], int, int&, int&, int&, int&);
void userMode(int[], int[], int[], int&, int&);
void checkBound(int);
int findInt(string);


//**EXECUTION BLOCK
int main(int argc, char *argv[])
{
    int pid;


    int pipe1[2]; //CPU writes, Memory reads
    int pipe2[2]; //CPU reads, Memory writes
    int state[2]; //CPU writes, Memory reads. 1 = read, 2 = write, 3 = exit

    if(argc != 3) //Based on the program guidelines, there should be 2 arguments on the command line. argc defaults to 1 with no arguments.
    {
        cout << "ERROR: Missing command line argument for either filename or value for interrupt processing." << endl;
        return 0;
    }
    if(pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(state) == -1) //Exit on pipe failure
    {
        cout << "ERROR: pipe failure\n";
        exit(0);
    }
    pid = fork();



    //**********************************
    //  ***************** MEMORY ZONE
    if(pid == 0)//child*/
    {
        ifstream file(argv[1]); //program to run from command line
        string lines; //holds each line of the program file

        int mode, address, y; //mode = read from state pipe, indicates what memory will do. address = index of memory that will be worked with
        int insWriter = 0; //counter for instantiating the instructions and operands into memory

        int memory [2000]; //The actual memory, where the data is held
        int ins = 0; //the instruction/operand read from the line of the file.


        for(int count = 0; count < 2000; count++)
        {
            memory[count] = -1;
            //cout << "memory instantiated.\n";

        }
        while(file.good()) //Reads to end of file. file.good() = 0 when there is nothing more to be read.
        {
            getline(file, lines); //read the line into 'lines'

            ins = findInt(lines); //Call function to find the first integer

            if(lines.at(0) == '.') //If the line is a change of load address, set counter to work there
                insWriter = ins;
            else if(ins == -1) //If no integer was found on the line, it isn't an instruction. Skip it
                continue;
            else //Integer found, no funny stuff
            {
                memory[insWriter] = ins; //Write instruction to memory

                insWriter++;
            }


        }
        //cout << "done with that\n";
        while(read(state[0], &mode, sizeof(int)) != -1) //Essentially infinite. Loop only ends if read fails somehow.
        {
            //cout << "Received Mode: " << mode << endl;
            read(pipe1[0], &address, sizeof(int)); //Get the address that will be worked with
            if(mode == 1) //read mode
            {
               //cout << "read: " << address << endl;
                //cout << "got value " << memory[address] << endl;
                write(pipe2[1],&(memory[address]), sizeof(int) ); //Write value contained in address
            }
            else if (mode == 2)//write mode
            {
                write(pipe2[1], &mode, sizeof(int)); //Indicate that memory is ready
                //cout <<"write: " << address << endl;

                read(pipe1[0], &ins, sizeof(int)); //Receive value to be written at the address
                memory[address] = ins; //write value
                //cout << "value: " << ins << endl;
                write(pipe2[1], &mode, sizeof(int)); //Indicate operation is done
            }
            else if (mode == 3) //If mode == 3, program is done
                return 0;
        }


    }




    // ****************
    // ************CPU ZONE
    else //parent process
    {
        int PC = 0; //Program Counter
        int SP = 999; //Stack Pointer
        int interBit = 0; //If 1, an interrupt is being processed. Prevents timer from incrementing
        int interrupt = 0; //If equal to interruptTimer, starts a timer interrupt
        int waitForChild; //Unused variable, used in read operations to stall CPU until memory is finished doing an operation
        int interruptTimer = atoi(argv[2]);//TO BE ASSIGNED BY COMMAND LINE

        //Constants for the modes, zero, and the addresses to start from during interrupts
        const int ZERO = 0;
        const int READ = 1;
        const int WRITE = 2;
        const int EXIT = 3;
        const int TIMER = 1000;
        const int INT = 1500;

        //Registers, and addr. addr is used for instructions that involve reading a memory address.
        int IR, AC, X , Y, addr;

        while(1) //infinite loop
        {

            write(state[1], &READ, sizeof(int)); //set memory mode
            //fprintf(stderr,"read signal\n");
            write(pipe1[1], &PC, sizeof(int)); //send address of next instruction/operand to memory
            //fprintf(stderr, "PC = %d\n", PC);
            read(pipe2[0], &IR, sizeof(int)); //read the value at PC's address in memory

            if(IR == 50) //50 = exit. end program
            {
                write(state[1], &EXIT, sizeof(int));
                exit(0);
            }
            switch(IR) //interpret instruction via switch case
            {
                case 1: PC++; //Load Value

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &AC, sizeof(int));
                    //fprintf(stderr, "AC %d \n", AC);
                    break;
                case 2: PC++; //Load addr

                    write(state[1], &READ, sizeof(int));
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &addr, sizeof(int)); //Read address from memory

                    if(interBit == 0) //If in user mode, make sure the address isn't in system memory
                        checkBound(addr);

                    write(state[1], &READ, sizeof(int)); //Read value at the address received into AC
                    write(pipe1[1], &addr, sizeof(int));
                    read(pipe2[0], &AC, sizeof(int));

                    //fprintf(stderr, "AC %d \n", AC);
                    break;
                case 3: PC++; //LoadInd addr

                    write(state[1], &READ, sizeof(int)); //First part is the same as Load addr
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &addr, sizeof(int));
                    if(interBit == 0)

                        checkBound(addr);

                    write(state[1], &READ, sizeof(int));
                    write(pipe1[1], &addr, sizeof(int));
                    read(pipe2[0], &addr, sizeof(int)); //Read that address at the address
                    if(interBit == 0)

                        checkBound(addr);

                    write(state[1], &READ, sizeof(int)); //Read value at THAT address into AC
                    write(pipe1[1], &addr, sizeof(int));
                    read(pipe2[0], &AC, sizeof(int));

                    break;
                case 4: PC++; //LoadIdxX addr

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &addr, sizeof(int));

                    addr += X; //offset address received by X
                    if(interBit == 0)
                        checkBound(addr);

                    write(state[1], &READ, sizeof(int)); //Read value at addr+X into AC
                    write(pipe1[1], &addr, sizeof(int));
                    read(pipe2[0], &AC, sizeof(int));

                    break;
                case 5: PC++; //LoadIdxY addr

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &addr, sizeof(int));

                    addr += Y; //offset by Y, otherwise the same as LoadIdxX
                    if(interBit == 0)
                        checkBound(addr);

                    write(state[1], &READ, sizeof(int));
                    write(pipe1[1], &addr, sizeof(int));
                    read(pipe2[0], &AC, sizeof(int));

                    break;
                case 6: addr = SP + X + 1; //LoadSpX, add 1 because I'm using a write-then-decrement approach
                    if(interBit == 0)
                        checkBound(addr);

                    write(state[1], &READ, sizeof(int)); //Read value at SP + X + 1 into AC
                    write(pipe1[1], &addr, sizeof(int));
                    read(pipe2[0], &AC, sizeof(int));

                    break;
                case 7: PC++;  //Store addr

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &addr, sizeof(int));

                    if(interBit == 0)
                        checkBound(addr);

                    write(state[1], &WRITE, sizeof(int)); //Tell memory to write at memory[addr]
                    write(pipe1[1], &addr, sizeof(int));
                    read(pipe2[0], &waitForChild, sizeof(int)); //Stall. Without this, memory might read values out of order and such

                    write(pipe1[1], &AC, sizeof(int)); //Write AC at memory[addr]
                    read(pipe2[0], &waitForChild, sizeof(int));

                    break;
                case 8: AC = getRandom(AC); //Get a random value between 1-100
                break;
                case 9: ++PC; //Put port

                    write(state[1], &READ, sizeof(int));
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &addr, sizeof(int));

                    putPort(AC, addr); //Print AC, format depends on value of addr
                    break;
                case 10: AC = addReg(AC, X); //AC = AC + X
                break;
                case 11: AC = addReg(AC, Y); //AC = AC + Y
                break;
                case 12: AC = subReg(AC, X); //AC = AC - X
                break;
                case 13: AC = subReg(AC, Y); //AC = AC -Y
                break;
                case 14: X = copyTo(AC, X); // X = AC
                //fprintf(stderr, "X = %d\n", X);
                break;
                case 15: AC = copyFrom(AC, X);// AC = X
                //fprintf(stderr, "X = %d\n", X);
                break;
                case 16: Y = copyTo(AC, Y); // Y = AC
                break;
                case 17: AC = copyFrom(AC, Y);//AC = Y
                break;
                case 18: SP = AC; //Set stack pointer to AC CopyToSp
                break;
                case 19: AC = SP; //Vice versa CopyFromSp
                break;
                case 20: PC++; //Jump addr

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &PC, sizeof(int)); //Jump to new address = PC points there

                    if(interBit == 0)
                        checkBound(PC);
                    PC--; //Decrement PC, because it will be incremented to the right value afterwards
                    break;
                case 21: //JumpIfEqual addr

                    if(AC == 0) //If AC = 0, do the same as Jump addr
                        {
                            //fprintf(stderr, "AC is 0");
                            PC++;

                            write(state[1], &READ, sizeof(int));//step forward & get val
                            write(pipe1[1], &PC, sizeof(int));
                            read(pipe2[0], &PC, sizeof(int));

                            if(interBit == 0)
                                checkBound(PC);
                            PC--;
                        }
                    else //If not, skip over the operand after the instruction
                        PC++;
                break;
                case 22: if(AC != 0) //If AC != 0 do Jump addr
                {
                    PC++;

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &PC, sizeof(int));
                    read(pipe2[0], &PC, sizeof(int));

                    if(interBit == 0)
                        checkBound(PC);
                    PC--;
                }
                else //skip operand
                    PC++;
                break;
                case 23:   write(state[1], &WRITE, sizeof(int)); //Call addr
                    write(pipe1[1], &SP, sizeof(int));
                    read(pipe2[0], &waitForChild, sizeof(int)); //Tell memory to write something to stack

                    write(pipe1[1], &PC, sizeof(int)); //Push PC to stack
                    read(pipe2[0], &waitForChild, sizeof(int));

                    SP--;
                    PC++;

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &PC, sizeof(int)); //set PC to jump address
                    read(pipe2[0], &PC, sizeof(int));

                    if(interBit == 0)
                        checkBound(PC);
                    PC--;
                    break;
                case 24: //Ret
                    SP++; //Write then decrement stack, so it must increment to read values

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &SP, sizeof(int));
                    read(pipe2[0], &PC, sizeof(int));//Pop PC from stack

                    write(state[1], &WRITE, sizeof(int));
                    write(pipe1[1], &SP, sizeof(int));
                    read(pipe2[0], &waitForChild, sizeof(int));

                    write(pipe1[1], &ZERO, sizeof(int)); //Clear out PC value from stack. Not necessarily needed, but it seems like good practice.
                    read(pipe2[0], &waitForChild, sizeof(int));
                    PC++;
                    break;
                case 25: X++; //incX
                break;
                case 26: X--; //decX
                break;
                case 27:  //Push
                    write(state[1], &WRITE, sizeof(int));
                    write(pipe1[1], &SP, sizeof(int));
                    read(pipe2[0], &waitForChild, sizeof(int));

                    write(pipe1[1], &AC, sizeof(int)); //Push AC onto stack
                    read(pipe2[0], &waitForChild, sizeof(int));

                    SP--;
                    break;
                case 28: //Pop
                    SP++;

                    write(state[1], &READ, sizeof(int));//step forward & get val
                    write(pipe1[1], &SP, sizeof(int));
                    read(pipe2[0], &AC, sizeof(int)); //Pop value from stack to AC

                    write(state[1], &WRITE, sizeof(int)); //Clear value from stack
                    write(pipe1[1], &SP, sizeof(int));
                    read(pipe2[0], &waitForChild, sizeof(int));

                    write(pipe1[1], &ZERO, sizeof(int));
                    read(pipe2[0], &waitForChild, sizeof(int));
                    break;
                case 29: //Int
                    kernelMode(state, pipe1, pipe2, INT, SP, interBit, interrupt, PC); //Syscall interrupt
                    PC--; //Decrement PC so that its at 1500 for the next iteration
                    break;
                case 30: //IRet
                    interBit = 0; //Reenables the interrupt timer
                    userMode(state, pipe1, pipe2, SP, PC); //Sets PC and SP back to previous values
                    PC++; //Prevents returning to an instruction's operand
                    break;



            }
            if(interBit == 0) //If not in an interrupt, increment timer
                interrupt++;

            //fprintf(stderr, "AC %d \n", AC);
            PC++; //Increment program counter
            if(interrupt == interruptTimer) //If there's a timer interrupt, it must be processed differently than syscall
            {

                interrupt = 0;
                PC--;
                kernelMode(state, pipe1, pipe2, TIMER, SP, interBit, interrupt, PC);
            }
        }
    }

    return 0;
}

int getRandom(int AC) //Returns a random int btwn 1 and 100
{
    srand(time(NULL));
    AC = rand() % 100 + 1;
    return AC;
}

int addReg(int AC, int reg) //Adds a register to AC
{
    return AC + reg;
}

int subReg(int AC, int reg)//Subtract a register from AC
{
    return AC - reg;
}
int copyTo(int AC, int reg)// Copy a register to AC.
{
    reg = AC;
    return reg;
}

int copyFrom(int AC, int reg) //Copy AC to register
{
    AC = reg;
    return reg;
}

void putPort(int AC, int port) //Print value in AC
{
   if (port == 1) //If port = 1, print integer value
    {
        fprintf(stderr, "%d", AC);
    }
    else //If port = 2, print ASCII equivalent of the integer value
    {

      char c = AC;
      fprintf(stderr, "%c", c);
    }

    return;
}

void kernelMode(int state[2], int pipe1[2], int pipe2[2], int interPoint, int &SP, int &interBit, int &interrupt, int &PC) //Converts to kernel mode
{
    int WRITE = 2;
     int temp = SP;
     int waitForChild;
    interBit = 1; //Set bit to 1 to prevent nested interrupts

    SP = 1999; //SP points to system stack

    write(state[1], &WRITE, sizeof(int));
    write(pipe1[1], &SP, sizeof(int));
    read(pipe2[0], &waitForChild, sizeof(int));

    write(pipe1[1], &temp, sizeof(int));
    read(pipe2[0], &waitForChild, sizeof(int)); //Write old SP to system stack

    SP--;

    write(state[1], &WRITE, sizeof(int));
    write(pipe1[1], &SP, sizeof(int));
    read(pipe2[0], &waitForChild, sizeof(int));

    write(pipe1[1], &PC, sizeof(int));
    read(pipe2[0], &waitForChild, sizeof(int)); //Write PC to system stack

    SP--;
    PC = interPoint; //= 1500 if syscall, = 1000 if timer
    return;
}

void userMode (int state[2], int pipe1[2], int pipe2[2], int &SP, int &PC) //return from kernel mode
{
    int zero = 0;
    int READ = 1;
    int WRITE = 2;
    SP++;

    write(state[1], &READ, sizeof(int));
    write(pipe1[1], &SP, sizeof(int));
    read(pipe2[0], &PC, sizeof(int)); //Pop old PC from system stack

    SP++;

    write(state[1], &READ, sizeof(int)); //Pop user stack pointer from system stack
    write(pipe1[1], &SP, sizeof(int));
    read(pipe2[0], &SP, sizeof(int));

    PC--;
    return;
}

void checkBound(int addr) //Check if user mode program is attempting to access system memory
{
    if(addr > 999)
    {
        cout << "ERROR: Attempt to access system memory in user mode.";
        exit(0);
    }
}

int findInt(string line) //Find first int in string
{
    const char *c = line.c_str(); //Convert c to c-string
    int val;
    for (;;) { //loop through c-string
        c += strcspn(c, "0123456789");//Finds the first occurrence of an integer
        if (*c == 0) return -1; //If there isn't one, return skip value
        if (sscanf(c, "%d", &val) == 1) { //sscanf will begin at the start of the integer, and read in the whole thing to val
            return val;
        }
        c++;


    }
}

