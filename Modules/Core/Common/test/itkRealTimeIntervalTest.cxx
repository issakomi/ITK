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

#include <iostream>
#include "itkRealTimeInterval.h"
#include "itkMacro.h"
#include "itkNumericTraits.h"
#include "itkMath.h"

#define CHECK_FOR_VALUE(a, b)                                                            \
  {                                                                                      \
    double eps = 4.0 * itk::NumericTraits<double>::epsilon();                            \
    ITK_GCC_PRAGMA_PUSH                                                                  \
    ITK_GCC_SUPPRESS_Wfloat_equal                                                        \
    eps = (b == 0.0) ? eps : itk::Math::abs(b * eps);                                    \
    ITK_GCC_PRAGMA_POP                                                                   \
    if (itk::Math::abs(a - b) > eps)                                                     \
    {                                                                                    \
      std::cerr << "Error in " #a << " expected " << b << " but got " << a << std::endl; \
      return EXIT_FAILURE;                                                               \
    }                                                                                    \
  }                                                                                      \
  ITK_MACROEND_NOOP_STATEMENT

#define CHECK_FOR_BOOLEAN(x, expected)          \
  {                                             \
    if ((x) != expected)                        \
    {                                           \
      std::cerr << "Error in " #x << std::endl; \
      return EXIT_FAILURE;                      \
    }                                           \
  }                                             \
  ITK_MACROEND_NOOP_STATEMENT


int
itkRealTimeIntervalTest(int, char *[])
{
  const itk::RealTimeInterval interval0;

  const double timeInMicroSeconds = interval0.GetTimeInMicroSeconds();
  const double timeInMilliSeconds = interval0.GetTimeInMilliSeconds();
  double       timeInSeconds = interval0.GetTimeInSeconds();
  const double timeInMinutes = interval0.GetTimeInMinutes();
  const double timeInHours = interval0.GetTimeInHours();
  const double timeInDays = interval0.GetTimeInDays();

  CHECK_FOR_VALUE(timeInMicroSeconds, 0.0);
  CHECK_FOR_VALUE(timeInMilliSeconds, 0.0);
  CHECK_FOR_VALUE(timeInSeconds, 0.0);
  CHECK_FOR_VALUE(timeInMinutes, 0.0);
  CHECK_FOR_VALUE(timeInHours, 0.0);
  CHECK_FOR_VALUE(timeInDays, 0.0);

  const itk::RealTimeInterval interval1;
  itk::RealTimeInterval       intervalX = interval0;

  const itk::RealTimeInterval oneSecond(1, 0);
  for (unsigned int i = 0; i < 1000000L; ++i)
  {
    intervalX += oneSecond;
  }

  std::cout << "intervalX = " << intervalX << std::endl;

  itk::RealTimeInterval manySeconds = intervalX - interval0;

  timeInSeconds = manySeconds.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, 1000000.0);

  itk::RealTimeInterval fiveMicroseconds;
  fiveMicroseconds.Set(0, 5);

  itk::RealTimeInterval interval3 = interval0;

  for (unsigned int i = 0; i < 1000000L; ++i)
  {
    interval3 += fiveMicroseconds;
  }

  manySeconds = interval3 - interval0;

  timeInSeconds = manySeconds.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, 5.0);

  for (unsigned int i = 0; i < 1000000L; ++i)
  {
    interval3 -= fiveMicroseconds;
  }

  manySeconds = interval3 - interval0;

  timeInSeconds = manySeconds.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, 0.0);


  itk::RealTimeInterval timeSpan;

  timeSpan.Set(19, -5000000L);

  timeInSeconds = timeSpan.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, 14.0);

  timeSpan.Set(-19, 5000000L);

  timeInSeconds = timeSpan.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, -14.0);

  timeSpan.Set(-19, -5000000L);

  timeInSeconds = timeSpan.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, -24.0);

  timeSpan.Set(19, 5000000L);

  timeInSeconds = timeSpan.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, 24.0);


  const itk::RealTimeInterval timeSpan1(19, 300000L);
  const itk::RealTimeInterval timeSpan2(13, 500000L);

  const itk::RealTimeInterval timeSpan3 = timeSpan1 + timeSpan2;

  timeInSeconds = timeSpan3.GetTimeInSeconds();

  CHECK_FOR_VALUE(timeInSeconds, 32.8);

  // Test comparison operations
  const itk::RealTimeInterval dt1(15, 13);
  const itk::RealTimeInterval dt2(19, 11);
  const itk::RealTimeInterval dt3(15, 25);

  CHECK_FOR_BOOLEAN(dt1 == dt1, true);
  CHECK_FOR_BOOLEAN(dt1 != dt2, true);
  CHECK_FOR_BOOLEAN(dt1 != dt1, false);
  CHECK_FOR_BOOLEAN(dt2 >= dt1, true);
  CHECK_FOR_BOOLEAN(dt1 >= dt1, true);
  CHECK_FOR_BOOLEAN(dt2 > dt1, true);
  CHECK_FOR_BOOLEAN(dt1 <= dt2, true);
  CHECK_FOR_BOOLEAN(dt1 <= dt1, true);
  CHECK_FOR_BOOLEAN(dt1 < dt2, true);

  CHECK_FOR_BOOLEAN(dt3 == dt3, true);
  CHECK_FOR_BOOLEAN(dt1 != dt3, true);
  CHECK_FOR_BOOLEAN(dt3 >= dt1, true);
  CHECK_FOR_BOOLEAN(dt3 > dt1, true);
  CHECK_FOR_BOOLEAN(dt3 <= dt1, false);
  CHECK_FOR_BOOLEAN(dt3 < dt1, false);
  CHECK_FOR_BOOLEAN(dt1 <= dt3, true);
  CHECK_FOR_BOOLEAN(dt1 < dt3, true);
  CHECK_FOR_BOOLEAN(dt1 >= dt3, false);
  CHECK_FOR_BOOLEAN(dt1 > dt3, false);


  std::cout << "[PASSED]" << std::endl;
  return EXIT_SUCCESS;
}
