/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkGEAdwImageIO.h"
#include "itkGE4ImageIO.h"
#include "itkGE5ImageIO.h"
#include "itkSiemensVisionImageIO.h"
#include "itkGEAdwImageIOFactory.h"
#include "itkGE4ImageIOFactory.h"
#include "itkGE5ImageIOFactory.h"
#include "itkSiemensVisionImageIOFactory.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"


// Specific ImageIO test

using ImageType = itk::Image<short, 3>;
using ImagePointer = ImageType::Pointer;
using ImageReaderType = itk::ImageFileReader<ImageType>;
using ImageWriterType = itk::ImageFileWriter<ImageType>;

int
itkGEImageIOFactoryTest(int argc, char * argv[])
{
  static bool firstTime = true;
  if (firstTime)
  {
    itk::ObjectFactoryBase::RegisterFactory(itk::GEAdwImageIOFactory::New());
    itk::ObjectFactoryBase::RegisterFactory(itk::GE4ImageIOFactory::New());
    itk::ObjectFactoryBase::RegisterFactory(itk::GE5ImageIOFactory::New());
    itk::ObjectFactoryBase::RegisterFactory(itk::SiemensVisionImageIOFactory::New());
    firstTime = false;
  }
  if (argc < 2)
  {
    return EXIT_FAILURE;
  }
  char * filename = *++argv;

  ImagePointer input;
  auto         imageReader = ImageReaderType::New();

  try
  {
    imageReader->SetFileName(filename);
    imageReader->Update();
    input = imageReader->GetOutput();
  }
  catch (const itk::ExceptionObject & e)
  {
    std::cout << "Caught unexpected exception. Test Failed!" << std::endl;
    std::cout << e << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int
itkGEImageIOTest(int argc, char * argv[])
{
  //
  // first argument is passing in the writable directory to do all testing
  if (argc > 1)
  {
    char * testdir = *++argv;
    --argc;
    itksys::SystemTools::ChangeDirectory(testdir);
  }

  if ((argc != 5) && (argc != 4))
  {
    return EXIT_FAILURE;
  }
  const std::string         failmode(argv[1]);
  const std::string         filetype(argv[2]);
  const std::string         filename(argv[3]);
  const bool                Failmode = failmode == std::string("true");
  itk::ImageIOBase::Pointer io;
  if (filetype == "GE4")
  {
    io = itk::GE4ImageIO::New();
  }
  else if (filetype == "GE5")
  {
    io = itk::GE5ImageIO::New();
  }
  else if (filetype == "GEAdw")
  {
    io = itk::GEAdwImageIO::New();
  }
  else if (filetype == "Siemens")
  {
    io = itk::SiemensVisionImageIO::New();
  }
  else
  {
    return EXIT_FAILURE;
  }

  ImagePointer input;
  auto         imageReader = ImageReaderType::New();

  try
  {
    imageReader->SetImageIO(io);
    imageReader->SetFileName(filename.c_str());
    imageReader->Update();
    input = imageReader->GetOutput();
  }
  catch (const itk::ExceptionObject & e)
  {
    if (Failmode)
    {
      std::cout << "Caught unexpected exception. Test Failed!" << std::endl;
      std::cout << e << std::endl;
      return EXIT_FAILURE;
    }
    else
    {
      std::cout << "Caught expected exception. Test Passed!" << std::endl;
      return EXIT_SUCCESS;
    }
  }

  if (failmode == std::string("true"))
  {
    auto writer = ImageWriterType::New();
    writer->SetInput(input);
    writer->SetFileName(argv[4]);
    writer->Update();
  }

  return Failmode ? 0 : 1;
}
