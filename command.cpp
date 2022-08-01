#include<bits/stdc++.h>
using namespace std;

bool create_file(string name,string path){
    //return true if the file is successfully create at a given destination
    /*
    Create File - ‘create_file <file_name> <destination_path>’
    Create Directory - ‘create_dir <dir_name> <destination_path>’
    a. Eg - create_file foo.txt ~/foobar
    create_file foo.txt .
    create_dir foo ~/foobar
    */

    //Traverse to the required directory

    fstream file;
    file.open(name,ios::out);
    if(!file){
        
    }
}

void commandMode(char *path)
{   

}

int main()
{
    return 0;
}