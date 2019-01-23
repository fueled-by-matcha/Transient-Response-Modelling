/*------------------------------------------------------------------
Program name: transientResponse.c
Authors: Lauren Johnson 
Description: This program tracks the transient response of a simple
chemical reactor and provides the analytical and Euler's solution.
---------------------------------------------------------------------*/
#include <stdio.h>
#include <gng1106plplot.h>  // provides definitions for using PLplot library
#include <math.h>
#include <float.h>

#define X_IX 0           // Row index for storing x values
#define FX_IX 1          // Row index for storing f(x) values
#define N 40             //Number of values that can be stored to savedData array from file
#define MAX 500          //Number of values
#define TRUE 1
#define FALSE 0
#define BIN_INPUT_FILE "reactor.bin"//name of file created by program
#define A 5 //number of values that can be stored in array of USERINPUT structures
typedef struct
{
    double q;//stores value for flow rate
    double cin;//stores value for initial conentration of substance entering the reactor (intake tube)
    double c0;//stores the value for the concentration at t=0
    double v;//stores the volume of the reactor
    double tf;//stores the final time for analysis
    double dt;//stores the time increment step
    int flag;//determines if the structure is filled
    int n;//time increment for euler
    double time[MAX];//stores time values
    double analytical[MAX];//stores concentration values calculated with the analytical method
    double euler[MAX];//stores concentration values calculated with the euler method
}USERINPUT;

// function prototypes
void selectData (USERINPUT [], USERINPUT *);
void getInput (USERINPUT *);
void printInput (USERINPUT *);
void calculateAnalytical (USERINPUT *);
void calculateEuler (USERINPUT *);
void plotSolutions (USERINPUT *);
double getMin (double [], int);
double getMax (double [], int);
void readBinFile (FILE *, USERINPUT []);
void saveToBinFile (FILE *, USERINPUT[], USERINPUT *);

/*---------------------------------------------------------------------
Function: main
Description: The main calls different functions that ask the user for either new values or
data from the reactor.bin file.  The main function then calls the calculateAnalytical and calculateEuler functions
to determine the solution.  The plotSolution function is then called to plot the solution and the printInput function
will display the input values provided by the user.  The user will then have the option of either saving the new data
to the file or closing the file
------------------------------------------------------------------------*/
void main()
{
   USERINPUT input;//structure used for calculations
   USERINPUT data [A];//array used to store user data from the file
   FILE *fp;//reference to the file
   char select;//stores user's letter choice
   int i;//controls loop that fills data array

   fp = fopen(BIN_INPUT_FILE, "rb");
   if(fp == NULL)//if file doesn't exist, create a new one and skip to grabbing new input
   {
       printf("File %s does not exist.  Please input new data\n", BIN_INPUT_FILE);
       for(i=0; i<A; i++)
        data[i].flag = 0;
        fwrite(data, sizeof(USERINPUT), A, fp);
       getInput(&input);
   }
   else//read the file and select data
   {
       readBinFile(fp, data);
       selectData(data, &input);
   }
   calculateAnalytical(&input);
   calculateEuler(&input);
   plotSolutions (&input);
   printInput(&input);

    printf("Would you like to save the new input values? (Y/N)\n");
       fflush(stdin);
       scanf("%c",&select);

   if(select == 'y')//save to file
    saveToBinFile(fp, data, &input);
   if(select=='n')
   {
     printf("The file has been closed.");
     fclose(fp);
   }



}
/*---------------------------------------------------------------------
Function: selectData
Parameters: -data: references the array that stores the data read from the binary
                   file as USERINPUT structures
            -*input: references a structure of type USERINPUT
Description: This function displays the data saved to the reactor.bin file
            for the user to select from.  The user then has the option of
            choosing a data set or creating a new one.  If the user chooses
            to use an existing data set, the *input structure will be updated
            with values from that data set
------------------------------------------------------------------------*/
void selectData(USERINPUT data[], USERINPUT *input)
{
    char symbol;//will store the letters y or n
    int choice; //will store the number for the data set that the user chooses
    int flag; //controls the do-while loop
    int i;//index values
    int count;//used to label the data sets
    count =1;

    for(i=0; i<5; i++)//display data stored in file
    {
      if(data[i].flag==1)
      {
          printf("data set %d\t", count);
          printf("q = %.3f\t", data[i].q);
          printf("c in = %.3f\t", data[i].cin);
          printf("c0 = %.3f\t", data[i].c0);
          printf("volume = %.3f\t", data[i].v);
          printf("final time = %.3f\t", data[i].tf);
          printf("time increment step = %.3f\t", data[i].dt);
          printf("\n\n");
      }
      count = count + 1;
    }
        printf("Would you like to use one of the stored data sets?(Y/N)\n");
        fflush(stdin);
        scanf("%c", &symbol);
    if (symbol == 'n')
        getInput(input);
    else
    {
        do//ensures that the user picks the correct data set
        {
          flag = TRUE;
          printf("Please choose a data set to use (1-5).\n");
          scanf("%d", &choice);
          if(choice <0 || choice > 5)
          {
              printf("Please re-enter a data set from 1-5\n");
              flag = FALSE;
          }
        }while (flag == FALSE);
        //fill input structure for calculations
        choice = choice-1;
        input->q = data[choice].q;
        input->cin = data[choice].cin;
        input->c0 = data[choice].c0;
        input->v = data[choice].v;
        input->tf = data[choice].tf;
        input->dt = data[choice].dt;
        input->n = data[choice].n;
        input->flag = 1;

    }
}
/*---------------------------------------------------------------------
Function: getInput
Parameters: -*input: references a structure of type USERINPUT
Description: This function asks the user for values that will be used to
            to calculate the analytical and Euler solutions.  The *input
            structure will be updated with these values.
------------------------------------------------------------------------*/
void getInput (USERINPUT *input)
{
    int flag;//controls do-while loop
    do//ensures that all values entered are valid
    {
        flag = TRUE;
        printf("Please enter a value for flow rate (m^3/min).\n");
        scanf("%lf", &input->q);
        printf("Please enter a value for the concentration of the input pipe (mg/ m^3).\n");
        scanf("%lf", &input->cin);
        printf("Please enter the initial concentration of substance in reactor (mg/m^3).\n");
        scanf("%lf", &input->c0);
        printf("Please enter the volume of the reactor (m^3).\n");
        scanf("%lf", &input->v);
        printf("Please enter the final time for transient response analysis in minutes.\n");
        scanf("%lf", &input->tf);
        printf("Please enter the time increment step in minutes.\n");
        scanf("%lf", &input->dt);
        input->n = (input->tf/input->dt);
        if(input->q<0||input->cin<0||input->c0<0||input->v<0||input->tf<0||input->dt<0||input->n>MAX)
        {
            printf("Please enter values greater than zero.\n");
            flag = FALSE;
        }
    }while(flag == FALSE);
    input->flag = 1;
}
/*---------------------------------------------------------------------
Function: printInput
Parameters: -*input: references a structure of type USERINPUT
Description: This function displays the data provided by the user.
------------------------------------------------------------------------*/
void printInput (USERINPUT *input)
{
    printf("The flow rate is %.3f m^3/min.\n", input->q);
    printf("The concentration of the input pipe is %.3f mg/m^3.\n", input->cin);
    printf("The initial concentration of the substance in the reactor is %.3f mg/m^3.\n", input->c0);
    printf("The volume of the reactor is %.3f m^3.\n", input->v);
    printf("The final time for transient analysis is %.3f minutes\n", input->tf);
    printf("The time increment step used is %.5f minutes\n", input->dt);
}
/*---------------------------------------------------------------------
Function: calculateAnalytical
Parameters: -*input: references a structure of type USERINPUT
Description: This calculates the analytical solution and fills the time[] and
            analytical[] members of the *input structure.
------------------------------------------------------------------------*/
void calculateAnalytical (USERINPUT *input)
{
    double x;//streamlines calculations
    double c;//stores temporary concentration values
    double inc;//increases time values
    int i;//counter for for-loop
    double t;//locally stores the time value

    x = -(input->q/input->v);
    t = 0;
    inc = input->tf/(MAX-1);

    for(i=0; i<MAX; i++)//fill analytical and time arrays
    {
        input->time[i] = t;
        c = input->cin*(1-exp(x*t))+ input->c0*exp(x*t);
        input->analytical[i] = c;
        t = t + inc;
    }
}
/*---------------------------------------------------------------------
Function: calculateEuler
Parameters: -*input: references a structure of type USERINPUT
Description: This calculates the euler solution and fills the euler[]
            member of the *input structure.
------------------------------------------------------------------------*/
void calculateEuler (USERINPUT *input)
{
    int i;//counter for for-loop
    input->euler[0]=input->c0;//initialise euler[0] for calculations
    for(i = 0; i <input->n; i++)
    {
        input->euler[i+1] = input->euler[i]+((input->q*input->cin-input->q*input->euler[i])/input->v)*input->dt;
    }

}
/*-------------------------------------------------
 Function: plotSolutions
 Parameters: -*input:references a structure of type USERINPUT
 Return value: none.
 Description: Initializes the plot.  The following values
              in the referenced structure are used to setup
              the plot:
	         time[], analytical[], euler []   range of horizontal axis
                 minC, maxC  - vertical axis range
	      Note the that the values of minFx and maxFx are determined with
	      the functions getMin and getMax.
              Then plots the analytical and euler curve on the same plot
-------------------------------------------------*/
void plotSolutions(USERINPUT *input)
{
    // Variable declaration
    double minC, maxC;  // Minimum and maximum values of f(x)
    double hold;
    double range;
    //determine the min and max for axis
    minC = getMin(input->analytical, MAX);
    maxC = getMax(input->analytical, MAX);
    range = maxC - minC;
    minC = minC + 0.1*range;
    minC = minC - 0.1*range;
    maxC = maxC+1;
    hold = getMax(input->euler, MAX);
    if(hold>maxC) maxC = hold;
    plsdev("wingcc");  // Sets device to wingcc - CodeBlocks compiler
    // Initialize the plot
    plinit();
    plwidth(2);
    plenv(0,input->tf,
	      minC, maxC, 0, 0);
    plcol0(GREEN);           // Select color for labels
    pllab("Time (minutes)", "Concentration(mg/m^3)", "Plot of Transient Response of a Simple Reactor");
        // Plot the analytical curve
    plcol0(RED);    // Color for plotting curve
    pllsty(SOLID);
    plline(MAX, input->time, input->analytical);
    plptex(0.1*input->tf, 0.9*maxC, 0, 0, 0, "Analytical");
        //Plot the Euler curve
    plcol0(BLUE);    // Color for plotting curve
    pllsty(LNGDASH_LNGGAP);
    plline(MAX, input->time, input->euler);
    plptex(0.3*input->tf, .9*maxC, 0, 0, 0, "Euler");
    plend();
}

/*-------------------------------------------------
 Function: getMin
 Parameters:
    analytical: reference to array of double values
    n: number of elements in the array
 Return value: the smallest value found in the array (min)
 Description: Finds the smallest value in the array.
              Uses a determinate loop to traverse the array
	      to test each value in the array.
-------------------------------------------------*/
double getMin(double analytical[], int n)
{
   // Variable declarations
   double min;  // for storing minimum value
   int ix;      // indexing into an array
   // Instructions
   min = analytical[0];  // most positive value for type double
   for(ix = 1; ix < n; ix = ix + 1)
   {
       if(min > analytical[ix]) min = analytical[ix];
   }
   return(min);
}

/*-------------------------------------------------
 Function: getMax
 Parameters:
    analytical: reference to array of double values
    n: number of elements in the array
 Return value: the largest value found in the array (max).
 Description: Finds the largest value in the array.
              Uses a determinate loop to traverse the array
	      to test each value in the array.
-------------------------------------------------*/
double getMax(double analytical[], int n)
{
   // Variable declarations
   double max;  // for storing maximum value
   int ix;      // indexing into an array
   // Instructions
   max = analytical[0];
   for(ix = 1; ix < n; ix = ix + 1)
   {
       if(max < analytical[ix]) max = analytical[ix];
   }
   return(max);
}
/*-------------------------------------------------
 Function: readBinFile
 Parameters: -*fp: the file pointer to the binary file created by the program
             -data: references the array that stores the data read from the binary file as USERINPUT structures
 Description: Reads what is contained in the reactor.bin file and stores it in the data array as USERINPUT structures
-------------------------------------------------*/
void readBinFile (FILE *fp, USERINPUT data [])
{
    fread(data,sizeof(USERINPUT), A, fp);
}
/*-------------------------------------------------
 Function: saveToBinFile
 Parameters: -*fp: the file pointer to the binary file created by the program
             -data: references the array that stores the data read from the binary file as USERINPUT structures
             -*input:references a structure of type USERINPUT
 Description: This function displays what is stored in the data array and asks the user
              to select a data set to overwrite.  The index of the chosen data set is then replaced
              with what is contained in the *input structure.  The data array is then written to the
              reactor.bin file.
-------------------------------------------------*/
void saveToBinFile(FILE *fp, USERINPUT data [], USERINPUT *input)
{
    int choice;//stores user choice for data set to oversrite
    int flag;//controls do-while loop
    int i;//controls for-loop for displaying data and replacing values in euler + analytical arrays
    int count;//used to label data
    count = 1;
    FILE *binfp;//pointer to binary file

    for(i=0; i<5; i++)//display the data
    {
      if(data[0].flag == 0)
        i = 4;
      if(data[i].flag==1)
      {
          printf("data set %d\t", count);
          printf("q = %.3f\t", data[i].q);
          printf("c in = %.3f\t", data[i].cin);
          printf("c0 = %.3f\t", data[i].c0);
          printf("volume = %.3f\t", data[i].v);
          printf("final time = %.3f\t", data[i].tf);
          printf("time increment step = %.3f\t", data[i].dt);
          printf("\n\n");
      }
      else
      {
          printf("data set %d\t", count);
          printf("\n\n");
      }
      count = count + 1;
    }
    do//select a data set to overwrite
    {
        flag = TRUE;
        printf("Please choose a data set to overwrite (1-5).\n");
        scanf("%d", &choice);
        if(choice <0 || choice >5)
        {
            printf("Please re-enter a data set from 1-5.\n");
            flag = FALSE;
        }
    }while(flag == FALSE);

    choice = choice-1;

    //replace the chosen structure in the data array
    data[choice].q = input->q;
    data[choice].cin = input->cin;
    data[choice].c0 = input->c0;
    data[choice].v = input->v;
    data[choice].tf = input->tf;
    data[choice].dt = input->dt;
    data[choice].flag = 1;
    data[choice].n = input->n;
    for(i=0; i<MAX; i++)//reset array values
    {
        input->time[i]= 0;
        input->analytical[i]=0;
        input->euler[i]=0;
    }

    binfp = fopen(BIN_INPUT_FILE, "wb");//save to binary file
    fwrite(data, sizeof(USERINPUT), A, binfp);
    fclose(binfp);
    printf("The data has been saved.");

}
