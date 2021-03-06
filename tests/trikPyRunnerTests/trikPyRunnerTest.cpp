/* Copyright 2014 - 2015 CyberTech Labs Ltd.
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

#include "trikPyRunnerTest.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFile>

#include <trikControl/brickFactory.h>
#include <trikKernel/fileUtils.h>
#include <testUtils/wait.h>
#include <QTimer>


using namespace tests;
constexpr auto EXIT_TIMEOUT = -93;
constexpr auto EXIT_SCRIPT_ERROR = 113;
constexpr auto EXIT_SCRIPT_SUCCESS = EXIT_SUCCESS;

void TrikPyRunnerTest::SetUp()
{
	mBrick.reset(trikControl::BrickFactory::create("./test-system-config.xml"
				   , "./test-model-config.xml", "./media/"));
	mScriptRunner.reset(new trikScriptRunner::TrikScriptRunner(*mBrick, nullptr));
	mScriptRunner->setDefaultRunner(trikScriptRunner::ScriptType::PYTHON);
	QObject::connect(&*mScriptRunner, &trikScriptRunner::TrikScriptRunnerInterface::sendMessage,
					 &*mScriptRunner, [this](const QString &m) { mStdOut += m + "\n"; });
// TODO:	mScriptRunner->registerUserFunction("assert", scriptAssert);
}

void TrikPyRunnerTest::TearDown()
{
}

int TrikPyRunnerTest::run(const QString &script)
{
	QEventLoop l;
	QTimer::singleShot(5000, &l, std::bind(&QEventLoop::exit, &l, EXIT_TIMEOUT));
	QObject::connect(&*mScriptRunner, &trikScriptRunner::TrikScriptRunnerInterface::completed
					 , &l, [&l](const QString &e) { l.exit(e.isEmpty() ? EXIT_SCRIPT_SUCCESS : EXIT_SCRIPT_ERROR); } );
	mStdOut.clear();
	mScriptRunner->run(script, "_.py");
	return l.exec();
}

int TrikPyRunnerTest::runDirectCommandAndWaitForQuit(const QString &script)
{
	QEventLoop l;
	QObject::connect(&*mScriptRunner, &trikScriptRunner::TrikScriptRunnerInterface::completed, &l, &QEventLoop::quit);
	mStdOut.clear();
	mScriptRunner->runDirectCommand(script);
	l.exec();
	return mScriptRunner->wasError()? EXIT_SCRIPT_ERROR : EXIT_SCRIPT_SUCCESS;
}

int TrikPyRunnerTest::runFromFile(const QString &fileName)
{
	auto fileContents = trikKernel::FileUtils::readFromFile("data/" + fileName);

#ifdef Q_OS_WIN
	fileContents = fileContents.replace("&&", ";");
#endif

	return run(fileContents);
}

trikScriptRunner::TrikScriptRunner &TrikPyRunnerTest::scriptRunner()
{
	return *mScriptRunner;
}

TEST_F(TrikPyRunnerTest, abortBeforeRun)
{
	scriptRunner().abortAll();
}

TEST_F(TrikPyRunnerTest, syntaxErrorReport)
{
	auto err = run("]");
	ASSERT_EQ(err, EXIT_SCRIPT_ERROR);
}

TEST_F(TrikPyRunnerTest, sanityCheck)
{
	auto err = run("1 + 1");
	ASSERT_EQ(err, EXIT_SCRIPT_SUCCESS);
	const auto &knownMethodNames = scriptRunner().knownMethodNames();
	ASSERT_TRUE(knownMethodNames.contains("brick"));
	ASSERT_TRUE(knownMethodNames.contains("setPower"));
	err = run("brick.motor('M2').setPower(10)");
	ASSERT_EQ(err, EXIT_SCRIPT_SUCCESS);
}

TEST_F(TrikPyRunnerTest, print)
{
	const QString text = "Hello";
	auto err = runDirectCommandAndWaitForQuit("print('" + text + "')");
	ASSERT_EQ(err, EXIT_SCRIPT_SUCCESS);
	auto const &out = mStdOut.split("print: ", QString::SplitBehavior::SkipEmptyParts)[0].trimmed();
	ASSERT_TRUE(text == out);
}

TEST_F(TrikPyRunnerTest, abortWhileTrue)
{
	QTimer t;
	t.setInterval(200);
	t.setSingleShot(true);
	using trikScriptRunner::TrikScriptRunnerInterface;
	QObject::connect(&scriptRunner(), &TrikScriptRunnerInterface::startedScript
					 , &t, QOverload<>::of(&QTimer::start));
	QObject::connect(&t, &QTimer::timeout, &scriptRunner(), &TrikScriptRunnerInterface::abort);
	auto err = run("print('before')\nwhile True: pass\nprint('after')");
	ASSERT_NE(err, EXIT_TIMEOUT);
	t.stop();
}

TEST_F(TrikPyRunnerTest, scriptWait)
{
	scriptRunner().run("script.wait(500)");
	tests::utils::Wait::wait(600);
}

TEST_F(TrikPyRunnerTest, directCommandContextWithTimersAndQtCore)
{
	auto err = runDirectCommandAndWaitForQuit("from PythonQt import QtCore");
	ASSERT_EQ(err, EXIT_SCRIPT_SUCCESS);
	err = runDirectCommandAndWaitForQuit("QtCore.QTimer.singleShot(100, lambda _ : None)");
	ASSERT_EQ(err, EXIT_SCRIPT_SUCCESS);
	err = runDirectCommandAndWaitForQuit("t=QtCore.QTimer()");
	ASSERT_EQ(err, EXIT_SCRIPT_SUCCESS);
}

TEST_F(TrikPyRunnerTest, propertyAndMethodWithSimpleType)
{
	auto exitCode = run("brick.gyroscope().read()");
	ASSERT_EQ(exitCode, EXIT_SCRIPT_SUCCESS);
}

TEST_F(TrikPyRunnerTest, brickMethodWithNonTrivialReturnTypeConversion)
{
	auto exitCode = run("brick.getStillImage()");
	ASSERT_EQ(exitCode, EXIT_SCRIPT_SUCCESS);
}

TEST_F(TrikPyRunnerTest, brickPropertyAndVectorArgument)
{
	auto exitCode = run("brick.display().show([0], 1, 1, 'grayscale8')");
	ASSERT_EQ(exitCode, EXIT_SCRIPT_SUCCESS);
}

TEST_F(TrikPyRunnerTest, DISABLED_fileTestPy)
{
	auto err = runFromFile("file-test.py");
	ASSERT_EQ(err, EXIT_SCRIPT_SUCCESS);
}

TEST_F(TrikPyRunnerTest, scriptExecutionControl)
{
	auto exitCode = run("a = script.timer(1000)");
	ASSERT_EQ(exitCode, EXIT_SCRIPT_SUCCESS);
}
