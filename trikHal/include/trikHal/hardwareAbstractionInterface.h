/* Copyright 2015 Yurii Litvinov and CyberTech Labs Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#pragma once

#include "inputDeviceFileInterface.h"
#include "outputDeviceFileInterface.h"
#include "eventFileInterface.h"
#include "fifoInterface.h"
#include "i2cInterface.h"
#include "systemConsoleInterface.h"

namespace trikHal {

class HardwareAbstractionInterface
{
public:
	virtual ~HardwareAbstractionInterface() {}
	virtual I2CInterface &i2c() = 0;
	virtual SystemConsoleInterface &systemConsole() = 0;

	virtual EventFileInterface *createEventFile() const = 0;
	virtual FifoInterface *createFifo() const = 0;
	virtual InputDeviceFileInterface *createInputDeviceFile(const QString &fileName) const = 0;
	virtual OutputDeviceFileInterface *createOutputDeviceFile(const QString &fileName) const = 0;
};

}
