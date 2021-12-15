#include <ros/ros.h>
#include <ros_msg_parser/ros_parser.hpp>
#include <matlogger2/matlogger2.h>
#include <matlogger2/mat_data.h>
#include <matlogger2/utils/mat_appender.h>

XBot::MatLogger2::Ptr logger;
std::map<std::string, std::map<std::string, std::vector<double>>> num_values;
std::map<std::string, std::map<std::string, std::string>> str_values;

void topicCallback(const RosMsgParser::ShapeShifter& msg,
                   std::string topic_name,
                   RosMsgParser::ParsersCollection& parsers)
{
  // you must register the topic definition.
  //  Don't worry, it will not be done twice
  parsers.registerParser(topic_name, msg);

  auto deserialized_msg = parsers.deserialize(topic_name, msg);

  // create the structure object that will be dumped with MatLogger2
  auto struct_data = XBot::matlogger2::MatData::make_struct();

  // Print the content of the message
  for (auto it : deserialized_msg->renamed_vals)
  {
    std::string& key = it.first;
    double value = it.second;
    std::replace(key.begin(), key.end(), '/', '_');
    num_values[topic_name][key.substr(topic_name.size()+1)].push_back(value);
  }
  for (auto it : deserialized_msg->flat_msg.name)
  {
    std::string key = it.first.toStdString();
    std::string value = it.second;
    std::replace(key.begin(), key.end(), '/', '_');
    str_values[topic_name][key.substr(topic_name.size()+1)] = value;
  }
}

// usage: pass the name of the topic as command line argument
int main(int argc, char** argv)
{
  RosMsgParser::ParsersCollection parsers;

  XBot::MatLogger2::Options opt;
  opt.default_buffer_size = 1e9;
  std::string environment = getenv("ROBOTOLOGY_ROOT");
  logger = XBot::MatLogger2::MakeLogger("/tmp/wrenches");//, opt);
  logger->set_buffer_mode(XBot::VariableBuffer::Mode::circular_buffer);

  if (argc == 1)
  {
    ROS_ERROR("Please specify at least one topic name!");
  }

  std::vector<std::string> topic_names;
  topic_names.resize(argc-1);
  for (int i = 0; i < argc-1; i++)
  {
      topic_names[i] = argv[i+1];

      // create a map for each topic to collect the data
      std::map<std::string, std::vector<double>> empty_num_map;
      std::map<std::string, std::string> empty_str_map;
      num_values.insert(std::make_pair(topic_names[i], empty_num_map));
      str_values.insert(std::make_pair(topic_names[i], empty_str_map));
  }

  ros::init(argc, argv, "logger_node");
  ros::NodeHandle nh;

  std::vector<boost::function<void(const RosMsgParser::ShapeShifter::ConstPtr&)>> callbacks;
  callbacks.resize(argc-1);
  for (int i = 0; i < argc-1; i++)
  {
    callbacks[i] = [&parsers, topic_names, i](const RosMsgParser::ShapeShifter::ConstPtr& msg) -> void
    {
        std::string topic_name = topic_names[i];
        topicCallback(*msg, topic_name, parsers);
    };
  }
  std::vector<ros::Subscriber> subscribers;
  subscribers.resize(argc-1);

  for (int i = 0; i < argc-1; i++)
    subscribers[i] = nh.subscribe(topic_names[i], 10, callbacks[i]);

  ros::Rate rate(30);
  while (ros::ok())
  {
    ros::spinOnce();
    rate.sleep();
  }

  // dump all data when the node is closed
  for (int i = 0; i < argc-1; i++)
  {
      auto struct_data = XBot::matlogger2::MatData::make_struct();
      for (auto num_map : num_values[topic_names[i]])
      {
        Eigen::VectorXd v = Eigen::VectorXd::Map(num_map.second.data(), num_map.second.size());
        struct_data[num_map.first] = v.transpose();
      }
      for (auto str_map : str_values[topic_names[i]])
        struct_data[str_map.first] = str_map.second;

      std::replace(topic_names[i].begin(), topic_names[i].end(), '/', '_');
      logger->save(topic_names[i].substr(1), struct_data);
  }

  logger.reset();
  return 0;
}
