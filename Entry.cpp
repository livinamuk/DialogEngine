
#include <string>
#include <vector>
#include <sstream> 
#include <windows.h>   // WinApi header for console color

#include "src\DialogEngine.h"


void ClearConsole() {
    COORD topLeft = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}

HANDLE  hConsole;
int red = 12;
int white = 7;
int blue = 11;
int yellow = 14;

void ShowInventory()
{
    std::cout << "\n";

    // Show flags
    SetConsoleTextAttribute(hConsole, yellow);
    std::cout << "FLAGS\n";
    SetConsoleTextAttribute(hConsole, white);
    for (GameFlag& gameflag : DialogEngine::s_gameFlags) {
        std::cout << " " << gameflag.m_name << ": ";
        if (gameflag.m_state == true)
            std::cout << "TRUE\n";
        else
            std::cout << "FALSE\n";
    }

    // Show values
    SetConsoleTextAttribute(hConsole, yellow);
    std::cout << "\nVALUES\n";
    SetConsoleTextAttribute(hConsole, white);
    for (GameFloat& gameFloat : DialogEngine::s_gameFloats) {
        std::cout << " " << gameFloat.m_name << ": " << gameFloat.m_value << "\n";
    }

    // Show items
    SetConsoleTextAttribute(hConsole, yellow);
    std::cout << "\nITEMS\n";
    SetConsoleTextAttribute(hConsole, white);
    for (GameItem& gameItem : DialogEngine::s_gameItems) {
        std::cout << " " << gameItem.m_name;

        if (gameItem.m_quantity > 1)
            std::cout << " " << gameItem.m_quantity;

        std::cout << "\n";
    }
}



/////////////////////////////////
//                             //
//   M A I N   P R O G R A M   //
//                             //
/////////////////////////////////

int main()
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    DialogEngine::LoadPlayerFile("playerdata.txt");
    DialogEngine::LoadDialogFile("dialog.txt");
    
    DialogEngine::SetCurrentDialogByID(1);

    //DialogEngine::SavePlayerFile("savefile.txt");

    //int nextDialogID = 1;
    bool gameRunning = true;
    HANDLE  hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    while (gameRunning)
    {
        // Clear console
        ClearConsole();

        // Show Inventory
        ShowInventory();

        // Show image and main text
        SetConsoleTextAttribute(hConsole, red);
        std::cout << "\n" << DialogEngine::GetCurrentDialogImageName() << ": " << DialogEngine::GetCurrentDialogText() << "\n\n";
       
        // show responses
        for (int i=0; i< DialogEngine::s_avaliableResponses.size(); i++)
        {
            Response response = DialogEngine::s_avaliableResponses[i];
            SetConsoleTextAttribute(hConsole, white);

            if (response.m_isSpecialColor)
                SetConsoleTextAttribute(hConsole, blue);

            std::cout << " " << (i + 1) << ") " << response.m_text << "\n";
        }

        // Get input
        int responseNumber = 0;
        while (responseNumber == 0)
        {
            std::cout << "\nEnter number: ";
            std::cin >> responseNumber; 
            std::cin.get();

            if (responseNumber > DialogEngine::s_avaliableResponses.size())
                responseNumber = 0;
        }

        // Set to yellow incase there is a Fusion action
        SetConsoleTextAttribute(hConsole, yellow);

        // Select the response
        DialogEngine::SelectResponse(responseNumber - 1);
                       
        //  Game over?
        if (DialogEngine::IsDialogOver())
            gameRunning = false;
    }

    SetConsoleTextAttribute(hConsole, white);
    std::cout << "\n";
    system("pause");
}

// 10pm-3am: 5 hours (1/22)
// 12pm-