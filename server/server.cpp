#include <zmq.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "frame.hpp"
#include <boost/program_options.hpp>
namespace po = boost::program_options;

const int ESC_KEY = 27;
int img_show(cv::Mat & frm, const std::string & fn, bool tracker, char ftype);

int main(int argc, char * argv[]) 
{
    std::string config_file;
    std::string address_port = "5555";

    char * homedir;
    if ((homedir = getenv("HOME")) == nullptr)
        homedir = getpwuid(getuid())->pw_dir;
    config_file = std::string(homedir) + "/.cvsinfo";

    // Declare a group of options that will be 
    // allowed only on command line
    po::options_description generic("Generic options");
    generic.add_options()
        ("help,h", "produce help message")
        ("config,c", po::value<std::string>(&config_file)->default_value(config_file),
            "name of a file of a configuration.")
    ;

    // Declare a group of options that will be 
    // allowed both on command line and in config file
    po::options_description config("Configuration");
    config.add_options()
        ("address_port", po::value<std::string>(&address_port)->default_value(address_port), 
            "Address port of server")
    ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config);

    po::options_description config_file_options;
    config_file_options.add(config);

    po::options_description visible("Allowed options");
    visible.add(generic).add(config);  

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, visible), vm);
    po::notify(vm);
    
    std::ifstream ifs(config_file.c_str());
    if (!ifs)
    {
        std::cerr << "Can not open config file: " << config_file << "\n";
        return EXIT_FAILURE;
    }
    else
    {
        po::store(parse_config_file(ifs, config_file_options), vm);
        po::notify(vm);
    }

    if (vm.count("help")) 
    {
        std::cout << visible << "\n";
        return EXIT_SUCCESS;
    }

    if (!vm.count("address_port"))
    {
        std::cerr << "Server address port was not set.\n";
        return EXIT_FAILURE;
    }

    address_port = vm["address_port"].as<std::string>();

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:" + address_port);
    
    while (true) 
    {
        Frame frame;
        receive(socket, frame);

        cv::Mat blob(frame.width, frame.height, CV_8UC3, frame.blob.data());
        std::string filename = frame.filename;
        bool tracker = frame.tracker;
        char filetype = frame.filetype;
        int retKey = img_show(blob, filename, tracker, filetype);

        //  Send reply back to client
        zmq::message_t reply(1);
        if (retKey == ESC_KEY)
            std::memcpy(reply.data(), "S", 1); // STOP
        else 
            std::memcpy(reply.data(), "R", 1); // RECEIVED
        socket.send(reply);
    }

    return 0;
}

int img_show(cv::Mat & frm, const std::string & fn, bool tracker, char ftype)
{
    cv::namedWindow(fn, cv::WINDOW_AUTOSIZE);
    cv::imshow(fn, frm);
    int retKey;
    if (ftype == '0')
        retKey = cv::waitKey(0) & 255;
    else if (ftype == '1')
        retKey = cv::waitKey(1) & 255;

    if ((retKey == ESC_KEY) || tracker)
        cv::destroyWindow(fn);

    return retKey;
}
