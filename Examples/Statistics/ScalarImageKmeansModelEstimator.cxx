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

//  Software Guide : BeginCommandLineArgs
//    INPUTS: {BrainT1Slice.png}
//  Software Guide : EndCommandLineArgs

// Software Guide : BeginLatex
//
// This example shows how to compute the KMeans model of a Scalar Image.
//
// The  \subdoxygen{Statistics}{KdTreeBasedKmeansEstimator} is used for taking
// a scalar image and applying the K-Means algorithm in order to define
// classes that represents statistical distributions of intensity values in
// the pixels. In the context of Medical Imaging, each class is typically
// associated to a particular type of tissue and can therefore be used as a
// form of image segmentation. One of the drawbacks of this technique is that
// the spatial distribution of the pixels is not considered at all. It is
// common therefore to combine the classification resulting from K-Means with
// other segmentation techniques that will use the classification as a prior
// and add spatial information to it in order to produce a better
// segmentation.
//
// Software Guide : EndLatex

#include "itkKdTree.h"
#include "itkKdTreeBasedKmeansEstimator.h"
#include "itkWeightedCentroidKdTreeGenerator.h"
#include "itkImageToListSampleAdaptor.h"

#include "itkImage.h"
#include "itkImageFileReader.h"
namespace
{
int
ExampleMain(int argc, const char * const argv[])
{

  if (argc < 2)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << argv[0] << " inputImageFileName" << std::endl;
    return EXIT_FAILURE;
  }

  using PixelType = unsigned char;
  constexpr unsigned int Dimension = 2;

  using ImageType = itk::Image<PixelType, Dimension>;

  ImageType::Pointer input;

  input = itk::ReadImage<ImageType>(argv[1]);


  // Software Guide : BeginCodeSnippet

  // Create a List from the scalar image
  using AdaptorType = itk::Statistics::ImageToListSampleAdaptor<ImageType>;

  auto adaptor = AdaptorType::New();

  adaptor->SetImage(input);

  // Create the K-d tree structure
  using TreeGeneratorType =
    itk::Statistics::WeightedCentroidKdTreeGenerator<AdaptorType>;
  auto treeGenerator = TreeGeneratorType::New();

  treeGenerator->SetSample(adaptor);
  treeGenerator->SetBucketSize(16);
  treeGenerator->Update();


  using TreeType = TreeGeneratorType::KdTreeType;
  using EstimatorType = itk::Statistics::KdTreeBasedKmeansEstimator<TreeType>;

  auto estimator = EstimatorType::New();

  constexpr unsigned int numberOfClasses = 3;

  EstimatorType::ParametersType initialMeans(numberOfClasses);
  initialMeans[0] = 25.0;
  initialMeans[1] = 125.0;
  initialMeans[2] = 250.0;

  estimator->SetParameters(initialMeans);

  estimator->SetKdTree(treeGenerator->GetOutput());
  estimator->SetMaximumIteration(200);
  estimator->SetCentroidPositionChangesThreshold(0.0);
  estimator->StartOptimization();

  EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();

  for (unsigned int i = 0; i < numberOfClasses; ++i)
  {
    std::cout << "cluster[" << i << "] " << std::endl;
    std::cout << "    estimated mean : " << estimatedMeans[i] << std::endl;
  }

  // Software Guide : EndCodeSnippet

  //  Software Guide : BeginLatex
  //
  // \begin{figure} \center
  // \includegraphics[width=0.44\textwidth]{BrainT1Slice}
  // \itkcaption[Output of the ScalarImageKmeansModelEstimator]{Test image for
  // the KMeans model estimator.}
  // \label{fig:ScalarImageKmeansModelEstimatorTestImage}
  // \end{figure}
  //
  //  The example produces means of 14.8, 91.6, 134.9 on
  // Figure \ref{fig:ScalarImageKmeansModelEstimatorTestImage}
  //
  //  Software Guide : EndLatex

  return EXIT_SUCCESS;
}
} // namespace
int
main(int argc, char * argv[])
{
  try
  {
    return ExampleMain(argc, argv);
  }
  catch (const itk::ExceptionObject & exceptionObject)
  {
    std::cerr << "ITK exception caught:\n" << exceptionObject << '\n';
  }
  catch (const std::exception & stdException)
  {
    std::cerr << "std exception caught:\n" << stdException.what() << '\n';
  }
  catch (...)
  {
    std::cerr << "Unhandled exception!\n";
  }
  return EXIT_FAILURE;
}
