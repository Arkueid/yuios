static char ascii_logo[][36] = {
    "\t__   ___   _ ___ ___  ___ \n",
    "\t\\ \\ / / | | |_ _/ _ \\/ __|\n",
    "\t \\ V /| |_| || | (_) \\__ \\\n",
    "\t  |_|  \\___/|___\\___/|___/\n",
};

#include <stdio.h>

int main()
{
    for (size_t i = 0; i < 4; i ++)
    {
        printf("%s", ascii_logo[i]);
    }
    return 0;
}

// static char ascii_logo[][36] = {
//     "\t__   ___   _ ___    ___  ____  \n",
//     "\t\\ \\ / / | | |_ _|  / _ \\/ ___| \n",
//     "\t \\ V /| | | || |  | | | \\___ \\ \n",
//     "\t  | | | |_| || |  | |_| |___) |\n",
//     "\t  |_|  \\___/|___|  \\___/|____/ \n",
// };

// #include <stdio.h>

// int main()
// {
//     for (size_t i = 0; i < 5; i ++)
//     {
//         printf("%s", ascii_logo[i]);
//     }
//     return 0;
// }