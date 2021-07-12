#include <iostream>
#include <experimental/filesystem>
#include <string>
#include <fstream>
namespace fs = std::experimental::filesystem;
 
int main(int argc, char *argv[])
{   
    std::string path_to_file="";
    std::string pid = argv[1];
    std::string pid_path = "/proc/"+pid+"/";
    std::string fd_path = pid_path+"fd/";
    std::string cmdline_path = pid_path + "cmdline";
    std::string cmdline_text;
    std::ifstream cmdline_file(cmdline_path);
    std::string tmp;
    while (std::getline (cmdline_file, tmp)) {
        cmdline_text.append(tmp);
    }
    //cmdline saves spaces as null terminated, so we put them back in
    for(int i = 0 ; i < cmdline_text.size(); i++){
        if(!cmdline_text[i]) cmdline_text[i] = ' ';
    }
    std::cout << cmdline_text << std::endl;
    // Close the file
    cmdline_file.close();
    //if there is no space it means the default path is given, which ends on test.txt, so lets find it
    auto pos_in_cmdline_text = cmdline_text.find(" ");
    if(pos_in_cmdline_text == std::string::npos){
        for (const auto & entry : fs::directory_iterator(fd_path)){
            if(fs::exists(entry)){
                if(fs::is_symlink(entry)){
                    std::string maybe_correct_file = fs::read_symlink(entry);
                    if(maybe_correct_file.find("test.txt")!=std::string::npos) {
                        path_to_file=maybe_correct_file;
                        std::cout << "found test.txt at: " << maybe_correct_file << '\n';
                        break;
                    }
                } else {
                    std::cout << "fd file exists but it is not a symlink\n";
                }
            } else {
                std::cout << "fd file does not exist\n";
            }
            /*
            fs::exists(entry)
            ? fs::is_symlink(entry)
            ? std::cout << fs::read_symlink(entry) << '\n')
            :  std::cout << " exists but it is not a symlink\n"
            :  std::cout << " does not exist\n";
            */
     }
     //this means we can get path of file from command
    } else {
        std::string argument="";
        auto end_pos_of_argument = cmdline_text.find(" ", pos_in_cmdline_text+1);
        argument=cmdline_text.substr(pos_in_cmdline_text,end_pos_of_argument);
        std::cout << "argument given: " << argument << std::endl;
        //that means we have a relative path and have to find the original path
        if(argument[0]!='/'){
             for (const auto & entry : fs::directory_iterator(pid_path+"exe")){
                if(fs::exists(entry)){
                    if(fs::is_symlink(entry)){
                        std::string original_path = fs::read_symlink(entry);
                        std::cout << "original path is: " << original_path << std::endl;
                        path_to_file=original_path;
                    } else {
                        std::cout << "exe file exists but it is not a symlink\n";
                    }
                } else {
                    std::cout << "exe does not exist\n";
                }
            }
        } else {
            path_to_file=argument;
        }
    }
    std::cout << "---------ALL-DATA-COLLECTED---------------" << std::endl;
    std::cout << "path fo tile: " << path_to_file << std::endl;
    std::cout << "------------------END---------------------" << std::endl;
        //std::cout << entry.path() << std::endl;
//    for(fs::path p : {"/proc/11125/fd/3", "/proc/11125/fd/0", "/proc/11125/fd/1", "/proc/11125/fd/2","/proc/11125/fd/19"}) {
        //std::cout << p;
        //    }
}