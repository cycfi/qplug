/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#pragma once

#include <qplug/controller.hpp>
#include <qplug/processor.hpp>
#include <elements.hpp>
#include <memory>

namespace elements = cycfi::elements;
namespace qplug = cycfi::qplug;

using dial_ptr = std::shared_ptr<elements::dial_base>;

class gain_controller : public qplug::controller
{
public:
                        gain_controller(base_controller& base);

      void              on_attach_view() override;
      parameter_list    parameters() override;

private:

   dial_ptr             _dial;
};

class gain_processor : public qplug::processor
{
public:
                        gain_processor(base_processor& base);
};

