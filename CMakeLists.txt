#
# Copyright (C) 2021 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
if(CONFIG_VIBRATOR)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES
             ${NUTTX_APPS_DIR}/frameworks/system/vibrator)

  nuttx_add_library(vibrator_api STATIC)

  list(APPEND CSRCS vibrator_api.c)

  if(CONFIG_VIBRATOR_SERVER)
    nuttx_add_application(
      NAME
      vibratord
      PRIORITY
      ${CONFIG_VIBRATOR_PRIORITY}
      STACKSIZE
      ${CONFIG_VIBRATOR_STACKSIZE}
      MODULE
      ${CONFIG_VIBRATOR}
      SRCS
      vibrator_server.c)
  endif()

  if(CONFIG_VIBRATOR_TEST)
    nuttx_add_application(
      NAME
      vibrator_test
      PRIORITY
      ${CONFIG_VIBRATOR_PRIORITY}
      STACKSIZE
      ${CONFIG_VIBRATOR_STACKSIZE}
      MODULE
      ${CONFIG_VIBRATOR}
      SRCS
      vibrator_test.c)
  endif()

  target_include_directories(vibrator_api PUBLIC .)
  target_sources(vibrator_api PRIVATE ${CSRCS})

endif()
