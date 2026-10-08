#include <stdio.h>
#include <stdlib.h>
#include "../include/common.h"

/*Instructor menu
• Login and logout from the system.  
• Class Schedule Management (add, view, search and update) - Manage training 
schedules and class availability, allowing students to book available classes 
conveniently 
• Issue Facility Problem (add and view) – Issue a facility problem such as damaged mats, 
broken equipment, lighting problems, air-conditioning issues, or damaged rooms. 
• View Instructor Rating (view and search) – View overall the rating score given by the 
students.*/
void instructorMenu(void)
{

    int opt = 0; // declare the opt in the outer scope fist.
    do
    {
        printf("Instructor Menu\n");
        printf("1. Add class\n");
        printf("2. View class\n");
        printf("3. Search class\n");
        printf("4. Update class\n");
        printf("5. Add issue facility problem\n");
        printf("6. View issue facility problem\n");
        printf("7. View ratings\n");
        printf("8. Log out\n");
    

        printf("Enter your choice: ");
         char choice[16]; 
        if (!fgets(choice, sizeof(choice), stdin)) break;
        opt = atoi(choice);

         switch (opt)
    {
        case 1:
            printf("[Add class - not implemented yet]\n");
            break;
        case 2:
            printf("[View class - not implemented yet]\n");
            break;
        case 3:
            printf("[Search class - not implemented yet]\n");
            break;
        case 4:
            printf("[Update class - not implemented yet]\n");
            break;
        case 5:
            printf("[Add issue facility problem - not implemented yet]\n");
            break;
        case 6:
            printf("[View issue facility problem- not implemented yet]\n");
            break;
        case 7:
            printf("[View ratings- not implemented yet]\n");
            break;
        case 8:
            printf("Logging out... \n");
            break;
        default:
            printf("Invalid value please try again\n");
    }
    } while (opt != 8);
}



    
