#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "frame.hpp"
#include <boost/program_options.hpp>
namespace po = boost::program_options;

void compress_frame(const cv::Mat & frame_in, cv::Mat & frame_out, 
    double resize_ratio, int jpeg_encoding_level);

int main(int argc, const char * argv[])
{
    try
    {
        std::string filetype;
        std::string filename;
        std::string config_file;
        std::string address_ip;
        std::string address_port;
        int jpeg_encoding_level;
        int frame_width;

        char * homedir;
        if ((homedir = getenv("HOME")) == nullptr)
            homedir = getpwuid(getuid())->pw_dir;
        config_file = std::string(homedir) + "/.cvcinfo";

        // Declare a group of options that will be 
        // allowed only on command line
        po::options_description generic("Generic options");
        generic.add_options()
            ("help,h", "produce help message")
            ("config,c", po::value<std::string>(&config_file)->default_value(config_file),
                "name of a file of a configuration.")
            ("filetype,t", po::value<std::string>(&filetype)->default_value("image"), 
                "type of input file, \"image\" or \"video\"")
        ;

        // Declare a group of options that will be 
        // allowed both on command line and in config file
        po::options_description config("Configuration");
        config.add_options()
            ("address_ip", po::value<std::string>(&address_ip)->default_value("127.0.0.1"), 
                "Address IP of server")
            ("address_port", po::value<std::string>(&address_port)->default_value("5555"), 
                "Address port of server")
            ("jpeg_encoding_level", po::value<int>(&jpeg_encoding_level)->default_value(95), 
                "Encoding level of JPEG, value between 0 - 100")
            ("frame_width", po::value<int>(&frame_width)->default_value(720), 
                "Frame width")
        ;

        // Hidden options, will be allowed both on command line and
        // in config file, but will not be shown to the user
        po::options_description hidden("Hidden options");
        hidden.add_options()
            ("filename", po::value<std::string>(), 
                "path to input file")
        ;

        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config).add(hidden);

        po::options_description config_file_options;
        config_file_options.add(config).add(hidden);

        po::options_description visible("Allowed options");
        visible.add(generic).add(config);
        
        po::positional_options_description p;
        p.add("filename", -1);    

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(cmdline_options).positional(p).run(), vm);
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

        if (!vm.count("filename")) 
        {
            std::cerr << "Filename was not set.\n";
            return EXIT_FAILURE;
        }

        if (!vm.count("address_ip"))
        {
            std::cerr << "Server address IP was not set.\n";
            return EXIT_FAILURE;
        }

        if (!vm.count("address_port"))
        {
            std::cerr << "Server address port was not set.\n";
            return EXIT_FAILURE;
        }

        filetype = vm["filetype"].as<std::string>();
        filename = vm["filename"].as<std::string>();
        address_ip = vm["address_ip"].as<std::string>();
        address_port = vm["address_port"].as<std::string>();
        jpeg_encoding_level = vm.count("jpeg_encoding_level") ? 
            vm["jpeg_encoding_level"].as<int>() : 95;
        frame_width = vm.count("frame_width") ? vm["frame_width"].as<int>() : 720; 

        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect("tcp://" + address_ip + ":" + address_port);

        if (filetype == "image")
        {
            cv::Mat frame_in = cv::imread(filename, cv::IMREAD_COLOR);
            cv::Mat frame_out;
            double ratio = 1.0 * frame_width / frame_in.cols;
            compress_frame(frame_in, frame_out, ratio, jpeg_encoding_level);
            if (!frame_out.empty())
            {
                send(socket, frame_out, filename, true, '0');
                std::cout << "sent\n";
            }
            else 
            {
                std::cout << "No data" << std::endl;
                return EXIT_FAILURE;
            }

            //  Get the reply.
            zmq::message_t reply;
            socket.recv(&reply);
        }
        else if (filetype == "video")
        {
            cv::VideoCapture cap(filename);
            if (!cap.isOpened())
            {
                std::cerr << "Error occurred when opening video file.\n";
                return EXIT_FAILURE;
            }

            int total_cnt = int(cap.get(cv::CAP_PROP_FRAME_COUNT));
            int cur_cnt = 0;
            double ratio;
            while (true)
            {
                cv::Mat frame_in;
                cap >> frame_in;
                if (frame_in.empty()) 
                    break;
                
                ratio = 1.0 * frame_width / frame_in.cols;
                cv::Mat frame_out;
                compress_frame(frame_in, frame_out, ratio, jpeg_encoding_level);

                if (cur_cnt < (total_cnt - 1))
                    send(socket, frame_out, filename, false, '1');
                else
                    send(socket, frame_out, filename, true, '1');    // last frame

                zmq::message_t reply;
                socket.recv(&reply);
                char retKey[1];
                std::memcpy(retKey, reply.data(), 1);
                if (retKey[0] == 'S')
                    break;

                cur_cnt++;
            }

            cap.release();
        }
        
    }
    catch(std::exception & e) 
    {
        std::cerr << "error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    catch(...) 
    {
        std::cerr << "Exception of unknown type!\n";
    }

    return EXIT_SUCCESS;
}

void compress_frame(const cv::Mat & frame_in, cv::Mat & frame_out, 
    double resize_ratio, int jpeg_encoding_level)
{
    cv::Mat resized_frame;
    cv::resize(frame_in, resized_frame, cv::Size(), resize_ratio, resize_ratio);

    std::vector<uchar> buf;
    std::vector<int> param = {cv::IMWRITE_JPEG_QUALITY, jpeg_encoding_level};
    cv::imencode(".jpg", resized_frame, buf, param);

    frame_out = cv::imdecode(buf, cv::IMREAD_COLOR).clone();
}
