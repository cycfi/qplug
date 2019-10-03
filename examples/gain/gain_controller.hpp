/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_CONTROLLER_JUNE_29_2019)
#define QPLUG_GAIN_CONTROLLER_JUNE_29_2019

#include <qplug/controller.hpp>
#include <elements.hpp>

namespace elements = cycfi::elements;
namespace qplug = cycfi::qplug;
using dial_ptr = std::shared_ptr<elements::dial_base>;

///////////////////////////////////////////////////////////////////////////////
class gain_controller : public qplug::controller
{
public:
                        gain_controller(base_controller& base);

      void              on_attach_view() override;
      parameter_list    parameters() override;

private:

   dial_ptr             _dial;
};

#endif
