#include "matlogger2/matlogger2.h"
#include "matio.h"

int main()
{
   auto logger = XBot::MatLogger2::MakeLogger("/tmp/my_log"); // date-time automatically appended
   logger->set_buffer_mode(XBot::VariableBuffer::Mode::circular_buffer);
   
   for(int i = 0; i < 1e3; i++)
   {
       Eigen::VectorXd vec(10);
       Eigen::MatrixXd mat(6,8);
       
       logger->add("my_vec_var", vec);
       logger->add("my_scalar_var", M_PI);
       logger->add("my_mat_var", mat);
   }

} // destructor flushes to disk