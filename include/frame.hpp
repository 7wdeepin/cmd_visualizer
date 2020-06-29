#ifndef FRAME_HPP_
#define FRAME_HPP_
#include <string>
#include <sstream>
#include <vector>
#include <zmq.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

typedef struct Frame
{
    int width;
    int height;
    std::vector<uchar> blob;
    std::string filename;
    bool tracker;
    char filetype;
} Frame;

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, Frame & frame, const unsigned int version)
{
    ar & frame.width;
    ar & frame.height;
    ar & frame.blob;
    ar & frame.filename;
    ar & frame.tracker;
    ar & frame.filetype;
}
}// end serialization
}// end boost

static void send(zmq::socket_t & skt, cv::Mat & src_mat, std::string & fn, bool tk, char ftype) 
{
    // cv::Mat to Frame
    Frame frame;
    frame.width = src_mat.rows;
    frame.height = src_mat.cols;
    cv::Mat flat = src_mat.reshape(1, src_mat.total() * src_mat.channels());
    frame.blob = src_mat.isContinuous() ? flat : flat.clone();
    frame.filename = fn;
    frame.tracker = tk;
    frame.filetype = ftype;

    // serialize frame
    std::ostringstream oss;
    boost::archive::text_oarchive serializer(oss);
    serializer << frame;
    auto serialized = oss.str();

    // send the serialized data via zmq socket
    zmq::message_t request(serialized.size());
    std::memcpy(request.data(), serialized.data(), serialized.size());
    skt.send(request);
} 

static void receive(zmq::socket_t & skt, Frame & frame)
{
    // wait for next request from client
    zmq::message_t request;
    skt.recv(&request);

    // deserialize received data
    std::string received(static_cast<char*>(request.data()), request.size());
    std::istringstream iss(received);
    boost::archive::text_iarchive deserializer(iss);
    deserializer >> frame;
}

#endif 