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
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkSimpleFilterWatcher.h"

#include "itkLabelImageToShapeLabelMapFilter.h"
#include "itkAttributePositionLabelMapFilter.h"
#include "itkTestingMacros.h"

/**
 * \class TestLabelObjectAccessor
 * \brief a test accessor - to make kwstyle happy
 */
template <typename TLabelObject>
class TestLabelObjectAccessor
{
public:
  using LabelObjectType = TLabelObject;
  using AttributeValueType = typename LabelObjectType::RegionType::IndexType;

  inline AttributeValueType
  operator()(const LabelObjectType * labelObject) const
  {
    return labelObject->GetBoundingBox().GetIndex();
  }
};


int
itkAttributePositionLabelMapFilterTest1(int argc, char * argv[])
{

  if (argc != 3)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv);
    std::cerr << " input output" << std::endl;
    return EXIT_FAILURE;
  }

  // declare the dimension used, and the type of the input image
  constexpr int dim = 3;
  using PType = unsigned char;
  using IType = itk::Image<PType, dim>;

  // We read the input image.
  using ReaderType = itk::ImageFileReader<IType>;
  auto reader = ReaderType::New();
  reader->SetFileName(argv[1]);

  // And convert it to a LabelMap, with the shape attribute computed.
  // We use the default label object type.
  using I2LType = itk::LabelImageToShapeLabelMapFilter<IType>;
  auto i2l = I2LType::New();
  i2l->SetInput(reader->GetOutput());

  using OpeningType =
    itk::AttributePositionLabelMapFilter<I2LType::OutputImageType,
                                         TestLabelObjectAccessor<I2LType::OutputImageType::LabelObjectType>,
                                         true>;
  auto opening = OpeningType::New();
  opening->SetInput(i2l->GetOutput());
  const itk::SimpleFilterWatcher watcher(opening, "filter");

  // the label map is then converted back to an label image.
  using L2IType = itk::LabelMapToLabelImageFilter<I2LType::OutputImageType, IType>;
  auto l2i = L2IType::New();
  l2i->SetInput(opening->GetOutput());

  // write the result
  using WriterType = itk::ImageFileWriter<IType>;
  auto writer = WriterType::New();
  writer->SetInput(l2i->GetOutput());
  writer->SetFileName(argv[2]);

  ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());


  return EXIT_SUCCESS;
}
