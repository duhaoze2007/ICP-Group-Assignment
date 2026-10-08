#include <stdio.h>
#include <stdlib.h>
#include "../include/common.h"

/*Student menu:
• Login and logout from the system. 
• Book Class Management (book, view, reschedule, and cancel) – Book classes made 
by the instructor. The student also can view, reschedule, and cancel the classes. 
• Payment Management (add and view) – Make payments for their registered classes 
and view their past payment records. 
• Instructor Rating (add and view) – Give and view the scoring rates on the instructor 
overall classes.*/

void studentMenu(void)
{

    int opt = 0; // declare the opt in the outer scope fist.
    do
    {
        printf("Student Menu\n");
        printf("1. Book class\n");
        printf("2. View bookings\n");
        printf("3. Reschedule Class\n");
        printf("4. Cancel Class\n");
        printf("5. Make Payment\n");
        printf("6. View payments\n");
        printf("7. Rate instructor\n");
        printf("8. View ratings\n");
        printf("9. Log out\n");

        printf("Enter your choice: ");
        //use the fgets + atoi for eveything keeping the input method consistent avoids this lefover-newline trap entirely,
        //since fgets always consumes the whole line, newline included, every single time.

        char choice[16]; //size [16] Reason: the buffer needs to safely handle input that doesn't match what you expet
        if (!fgets(choice, sizeof(choice), stdin)) break;//we used fgets with atoi to avoid cleanup line after every scanf.
        opt = atoi(choice);//reassign opt as the atoi(choice)

    switch (opt)
    {
        case 1:
            printf("[Book class - not implemented yet]\n");
            break;
        case 2:
            printf("[View bookings - not implemented yet]\n");
            break;
        case 3:
            printf("[ Reschedule Class - not implemented yet]\n");
            break;
        case 4:
            printf("[Cancel Class - not implemented yet]\n");
            break;
        case 5:
            printf("[Make Payment - not implemented yet]\n");
            break;
        case 6:
            printf("[View payments - not implemented yet]\n");
            break;
        case 7:
            printf("[Rate instructor - not implemented yet]\n");
            break;
        case 8:
            printf("[View ratings - not implemented yet]\n");
            break;
        case 9:
            printf("Logging out...\n");
            break;
        default:
            printf("Invalid input. please try again.\n");
    }

    } while (opt != 9);
}
