#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <limits.h>
#include <string>
#include <dirent.h> //DS to maintain file
#include <stack>
#include <time.h> //ctime for modification time
#include <stdio.h>
#include <sys/stat.h> //meta data about file
#include <pwd.h>      // file owner
#include <grp.h>      //group owner
#include <vector>
#include <cstring>
#include <sys/ioctl.h> //max no. of row in terminal for linux
#include <termios.h>   //for raw mode
#include <algorithm>   //sort
#include <sys/types.h> //pipes
#include <fcntl.h>     // for redirection of output
#include <fstream>
// recheck

using namespace std;

// struct used by ioctl to give max no. of rows printed on terminal
struct winsize w;

// move cursor to x and y coordinate
//#define moveCursor(x,y) printf("\033[%d;%dH",x,y);

#define moveCursor(x, y) cout << "\033[" << (x) << ";" << (y) << "H";
#define clrscr() printf("\033[H\033[J") // clear screen based on escape sequence

struct meta
{
    string homePath;
    stack<string> backward, forward, searchStack; // used for going back and forword
    vector<string> files;
    int maxrow;
    int MAXN;       // no of files can be printed based on the screen size
    int firstIndex; // point to first index of files
    int lastIndex;  // point to last index of files
    int cursorptr;
    string currentDir;
    vector<string> inputVector;
    bool foundFile;
} ds;

void initialise(char *path)
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // initalize value of w based on screen
    ds.maxrow = w.ws_row;
    ds.MAXN = ds.maxrow - 6;
    ds.cursorptr = 1;
    ds.firstIndex = 0;
    ds.lastIndex = ds.MAXN;
    ds.homePath = path;
}

void printMetaFile()
{
    // https://linux.die.net/man/2/lstat
    // Function to print file properties given file path. Simillar to ls -l

    // cout << "\e[8;38;100t";  //resizing terminal

    clrscr();
    // set terminal width to 50, height to 100

    int index = ds.firstIndex;
    while (index < min((int)ds.files.size(), ds.firstIndex + ds.MAXN))
    {
        dirent *a;
        struct stat stats;

        lstat(ds.files[index].c_str(), &stats); // path,buffer

        // print permission
        if (S_ISDIR(stats.st_mode))
            cout << "d";
        else
            cout << "-";
        if (stats.st_mode & S_IRUSR)
            cout << "r";
        else
            cout << "-";
        if (stats.st_mode & S_IWUSR)
            cout << "w";
        else
            cout << "-";
        if (stats.st_mode & S_IXUSR)
            cout << "x";
        else
            cout << "-";
        if (stats.st_mode & S_IRGRP)
            cout << "r";
        else
            cout << "-";
        if (stats.st_mode & S_IWGRP)
            cout << "w";
        else
            cout << "-";
        if (stats.st_mode & S_IXGRP)
            cout << "x";
        else
            cout << "-";
        if (stats.st_mode & S_IROTH)
            cout << "r";
        else
            cout << "-";
        if (stats.st_mode & S_IWOTH)
            cout << "w";
        else
            cout << "-";
        if (stats.st_mode & S_IXOTH)
            cout << "x";
        else
            cout << "-";

        // owner name
        struct passwd *pw = getpwuid(stats.st_uid);
        if (pw != nullptr)
            cout << "\t" << pw->pw_name << " ";

        // group name
        struct group *gr = getgrgid(stats.st_gid);
        if (gr != nullptr)
            cout << gr->gr_name << " ";

        // Size of the file
        printf("%10.2fK  ", stats.st_size / 1024.0);
        string timeDate = (ctime(&stats.st_mtime));

        timeDate.pop_back(); // extra new line character is appending at the end

        cout << timeDate << "   ";
        // cout << ds.files[index].c_str() << "\n";
        printf("%.40s\n", ds.files[index].c_str());
        index++;
    }

    moveCursor(w.ws_row - 4, 0);
    // moveCursor(min((int)ds.files.size(), ds.lastIndex),0);
    cout << "Normal Mode, Press : to switch to command mode\n";
}

void setFileVector(string cwd)
{
    struct dirent *ent;
    DIR *dir = opendir(cwd.c_str());
    if (dir == nullptr)
        return;
    chdir(cwd.c_str()); // change working directory to dir
    ds.files.clear();   // remove old files on previous directory
    char path[PATH_MAX];
    getcwd(path, PATH_MAX);
    ds.currentDir = path;

    /* put all the files and directories within directory */
    while (dir && (ent = readdir(dir)) != NULL)
    {
        ds.files.push_back(ent->d_name);
    }
    closedir(dir);

    ds.firstIndex = 0;
    ds.cursorptr = min(ds.MAXN, int(ds.files.size()));
    ds.lastIndex = min(ds.MAXN, int(ds.files.size()));
    sort(ds.files.begin(), ds.files.end());

    printMetaFile();
    moveCursor(ds.cursorptr, 0);

    // if (dir == nullptr)
    //     cout << "Wrong Path Provided";
}

void die(const char *s)
{
    cerr << s << "\n";
    exit(1);
}

struct termios orig_termiosSettings, newSettings;

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termiosSettings) == -1)
        die("tcsetattr");
}

void enableRawMode()
{
    // Reference https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
    if (tcgetattr(0, &orig_termiosSettings) == -1)
        die("tcgetattr");
    newSettings = orig_termiosSettings;

    // echo - each key enterd will be printed
    // ICANON flag that allows us to turn off canonical mode.
    // This means we will finally be reading input byte-by-byte, instead of line-by-line.
    // ISIG - Turns off clt+c and clt+z
    //  IEXTEN - Disable Ctl+V

    // newSettings.c_iflag &= ~(BRKINT);
    // When BRKINT is turned on, a break condition will cause a
    // SIGINT signal to be sent to the program, like pressing Ctrl-C.
    // newSettings.c_iflag &= ~(ICRNL); //  ICRNL - mix clt+M & read it as 13,without it will read it as 10
    // newSettings.c_iflag &= ~(IXON);  // IXON - Disable Ctrl-S and Ctrl-Q
    // newSettings.c_iflag &= ~(INPCK);
    // INPCK enables parity checking, which doesn’t seem to apply to modern terminal emulators.
    // newSettings.c_iflag &= ~(ISTRIP);
    // ISTRIP causes the 8th bit of each input byte to be stripped,
    //  meaning it will set it to 0. This is probably already turned off.

    newSettings.c_lflag &= (~(ECHO | ICANON));
    // newSettings.c_lflag &= ~(IEXTEN | ISIG);

    newSettings.c_cc[VMIN] = 1;
    newSettings.c_cc[VTIME] = 0;

    // The VMIN value sets the minimum number of bytes of input needed before read() can return.
    // We set it to 0 so that read() returns as soon as there is any input to be read.
    // The VTIME value sets the maximum amount of time to wait before read() returns.
    // It is in tenths of a second, so we set it to 1/10 of a second, or 100 milliseconds.
    // If read() times out, it will return 0, which makes sense because its usual return value is the
    // number of bytes read.
    if (tcsetattr(0, TCSANOW, &newSettings) == -1)
        // TCSAFLUSH argument specifies when to apply the change
        die("tcsetattr");
}

void scrollUp()
{
    if (ds.cursorptr > 1)
    {
        --ds.cursorptr;
        moveCursor(ds.cursorptr, 0);
    }
    // else if(ds.firstIndex!=0){
    // scope of going up
    //    ds.lastIndex--;
    //    ds.firstIndex--;
    //    printMetaFile();
    //    moveCursor(ds.cursorptr,0);
    //}
}

void scrollDown()
{
    if (ds.cursorptr < ds.files.size() && ds.cursorptr < ds.MAXN)
    {
        ++ds.cursorptr;
        moveCursor(ds.cursorptr, 0);
    }
    // else if(ds.lastIndex < ds.files.size()-1) {
    //     ++ds.firstIndex;
    //     ++ds.lastIndex;
    //     printMetaFile();
    //     moveCursor(ds.cursorptr, 0);
    // }
}

void goBack()
{
    // checks if we can go back
    if (ds.backward.size() == 0)
        return;
    string newPath = ds.backward.top();
    ds.backward.pop();
    ds.forward.push(ds.currentDir);
    setFileVector(newPath);
}

void goForward()
{

    // checks if we can go forward
    if (ds.forward.size() == 0)
        return;
    string newPath = ds.forward.top();
    ds.forward.pop();
    ds.backward.push(ds.currentDir);
    setFileVector(newPath);
}

void goUp()
{
    // check if already present in base folder or not
    if (strcmp(ds.currentDir.c_str(), "/") == 0)
        return;
    ds.backward.push(ds.currentDir);
    setFileVector("../");
}
void goHome()
{
    // checks if currentDir is not rootDir

    if (strcmp(ds.currentDir.c_str(), ds.homePath.c_str()) == 0)
        return; // already in home
    ds.backward.push(ds.currentDir);
    setFileVector(ds.homePath);
}

void Open()
{

    const char *fileName = ds.files[ds.firstIndex + ds.cursorptr - 1].c_str();
    // get the name of the file
    struct stat stats;
    lstat(fileName, &stats); // returns info about fileName

    if (!S_ISDIR(stats.st_mode))
    {
        // open file using default application
        pid_t pid = fork();
        // creatin two prcoess s.t. both the application will run
        if (pid == 0) // child
        {
            // int file_desc = open("tricky.txt",O_WRONLY | O_APPEND);

            // here the newfd is the file descriptor of stdout (i.e. 1)
            // dup2(file_desc, 1) ;

            execl("/usr/bin/xdg-open", "xdg-open", fileName, NULL);
            exit(1);
            // after closing of the file kill the child
        }
    }
    else //  it is a directory
    {
        // if user pressed . then return
        if (strcmp(fileName, ".") == 0)
            return;

        // if the user pressed .. and no parent directory avaiable return
        if (strcmp(fileName, "..") == 0 && strcmp(ds.currentDir.c_str(), "/") == 0)
            return;

        // push current dir in backStack
        ds.backward.push(ds.currentDir);
        // enter that directory
        setFileVector((ds.currentDir) + '/' + string(fileName));
    }
    // ds.currentDir
}

// clears CMD of command mode
void clean()
{
    moveCursor(w.ws_row - 3, 0);

    for (int i = w.ws_row - 3; i < w.ws_row; ++i)
    {
        for (int j = 0; j < w.ws_col; ++j)
            cout << " ";
    }
    moveCursor(w.ws_row - 3, 0);
}

void tokenisize(string input)
{
    ds.inputVector.clear();

    string temp;
    bool backFound = false;
    for (int i = 0; i < input.size(); ++i)
    {
        char c = input[i];
        if (backFound)
        {
            backFound = false;
        }
        else if (c == ' ')
        {
            ds.inputVector.push_back(temp);
            temp = "";
        }
        else if (c == 92)
        { //     '\' backward slash
            temp += " ";
            backFound = true; // avoid some escape sequence
        }
        else
            temp.push_back(c);
    }
    if (temp.size() > 0)
        ds.inputVector.push_back(temp);
}
bool searchFiles(string path, string fileName)
{

    // cout<<path<<"\n";
    DIR *dp = opendir(path.c_str());
    if (dp == nullptr)
    {
        return false;
    }
    struct dirent *entry;
    struct stat statbuf;
    bool find = false;
    chdir(path.c_str()); //  i m in a
    while ((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            if (strcmp(entry->d_name, fileName.c_str()) == 0)
                return true;
            else
            {
                if (!find)
                    find = searchFiles(path + '/' + string(entry->d_name), fileName);
            }
        }
        else
        {
            // file not direcotry
            // cout<<fileName<<" ";
            if (strcmp(fileName.c_str(), entry->d_name) == 0)
            {
                return true;
            }
        }
    }
    chdir("..");
    return find;
}

/*
void copyContent(string fileName, string dest) {
    char block[PATH_MAX];
    int nread;
    long in = open(fileName.c_str(), O_RDONLY);
    long out = open((dest + '/' + fileName).c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    while ((nread = read( in , block, sizeof(block))) > 0) {
        write(out, block, nread);
    }



}*/

void switchMode()
{
    clean();
    cout << "Mode: Command Mode. Press ESC to switch to Normal Mode\n";

    char keypress;
    while (true)
    {
        string command;
        cout << "$";
        enableRawMode();
        keypress = getchar();
        disableRawMode();

        while (keypress != 27 && keypress != 10) // escape and enter resp.
        {
            cout << keypress;
            if (keypress == 127 && command.size() > 0)
            { // backspace
                command.pop_back();
                cout << "\b \b";
            }
            command.push_back(keypress);
            enableRawMode();
            keypress = getchar();
            disableRawMode();
        }
        if (keypress == 27)
        {
            // escape character
            setFileVector(ds.currentDir);
            return;
        }
        tokenisize(command);

        // search the file
        if (ds.inputVector[0] == "search")
        {
            string fileName = ds.inputVector[1];
            char cwd[PATH_MAX]; // 4096 in my case
            getcwd(cwd, PATH_MAX);

            if (searchFiles(cwd, fileName))
            {
                cout << "\nTrue";
            }
            else
                cout << "\nFalse";
        }
        else if (ds.inputVector[0] == "goto")
        {
            if (ds.inputVector[1][0] == '~')
            {
                setFileVector(ds.homePath + ds.inputVector[1].substr(1));
            }
            else
                setFileVector((ds.inputVector[1]));
            // if path is wrong and with space in bw it will keep showing current window
            moveCursor(ds.cursorptr, 0);
        }
        else if (ds.inputVector[0] == "rename")
        {
            string a = ds.inputVector[1];
            string b = ds.inputVector[2];
            rename(a.c_str(), b.c_str());
            cout << "\n";
            for (int i = 1; i < ds.inputVector.size(); ++i)
                cout << ds.inputVector[i] << " ";
            cout << "Renamed Succesfully!\n";
        }
        else if (ds.inputVector[0] == "create_file" || ds.inputVector[0] == "create_dir")
        {
            int size = ds.inputVector.size();
            string location = "";

            if (ds.inputVector[size - 1][0] == '~')
                location = ds.homePath + ds.inputVector[size - 1].substr(1);
            else if (ds.inputVector[size - 1][0] == '.')
                location = ds.currentDir;
            else if (ds.inputVector[size - 1][0] == '/')
                location = ds.currentDir + ds.inputVector[size - 1];
            else
            {
                cout << "\nwrong Path!\n";
                continue;
            }

            for (int i = 1; i < size - 1; i++)
            {
                string fileName = ds.inputVector[i];
                if (ds.inputVector[0] == "create_dir")
                {
                    mkdir((location + '/' + fileName).c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
                    cout << "\nDirecotry Created Succesfully!\n";
                }
                else
                {
                    open((location + '/' + fileName).c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                    cout << "\nFile Created Succesfully!\n";
                }
            }
        }
        else if (ds.inputVector[0] == "copy")
        {

            int size = ds.inputVector.size();
            string fileName, location;

            if (ds.inputVector[size - 1][0] == '~')
                location = ds.homePath + ds.inputVector[size - 1].substr(1);
            else if (ds.inputVector[size - 1][0] == '.')
                location = ds.currentDir;
            else if (ds.inputVector[size - 1][0] == '/')
                location = ds.currentDir + ds.inputVector[size - 1];
            else
            {
                cout << "\nwrong Path!\n";
                continue;
            }

            for (int i = 1; i < size - 1; ++i)
            {
                fileName = ds.inputVector[i];
                std::ifstream src(fileName, std::ios::binary);
                std::ofstream dest(location + fileName, std::ios::binary);

                struct stat st;
                stat(fileName.c_str(), &st);
                chmod((location + fileName).c_str(), st.st_mode);

                dest << (src).rdbuf();
            }
            cout << "\nCopied Succesfully!";
        }
        // formating
        sleep(3);
        clean();
    }
}

// BKSPACE:127       ESC:27
int main()
{
    char cwd[PATH_MAX]; // 4096 in my case
    getcwd(cwd, PATH_MAX);
    initialise(cwd);
    setFileVector(cwd);
    // printMetaFile();
    moveCursor(ds.cursorptr, 0);
    char keypress = 'D'; // any value other than q and Q

    while (keypress != 'q' && keypress != 'Q')
    {
        enableRawMode();
        keypress = getchar();
        disableRawMode();

        if ((keypress == 'k' || keypress == 'K') && ds.firstIndex != 0)
        {
            // upward scrolling using k
            ds.firstIndex--;
            ds.lastIndex--;
            printMetaFile();
            moveCursor(ds.cursorptr, 0);
        }
        else if (keypress == 65) // 27 91 65 AND UP-ARROW KEY = [[A
        {
            // scroll up
            scrollUp();
        }
        else if (keypress == 66) // KEY_DOWN:27 91 66
        {
            // scroll Down
            scrollDown();
        }
        else if ((keypress == 'l' || keypress == 'L') && ds.lastIndex != ds.files.size())
        {
            // downward scrolling using L
            ++ds.firstIndex;
            ++ds.lastIndex;
            printMetaFile();
            moveCursor(ds.cursorptr, 0);
        }
        else if (keypress == 127) // backspace
        {
            // go up one level on backspace
            goUp();
        }
        else if (keypress == 68)
        {
            // go back on left arrow
            goBack();
        }
        else if (keypress == 67)
        {
            // go forward on right arrow
            goForward();
        }
        else if (keypress == 'h' || keypress == 'H')
        {
            // go to root path of project
            goHome();
        }
        else if (keypress == ':')
        {
            switchMode();
        }
        else if (keypress == 10) // ENTER
        {
            Open(); // open file
        }
    }
    clrscr();
    return 0;
}