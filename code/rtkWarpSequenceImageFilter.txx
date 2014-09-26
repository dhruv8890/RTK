/*=========================================================================
 *
 *  Copyright RTK Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef __rtkWarpSequenceImageFilter_txx
#define __rtkWarpSequenceImageFilter_txx

#include "rtkWarpSequenceImageFilter.h"

namespace rtk
{

template< typename TImageSequence, typename TMVFImageSequence, typename TImage, typename TMVFImage>
WarpSequenceImageFilter< TImageSequence, TMVFImageSequence, TImage, TMVFImage>
::WarpSequenceImageFilter()
{
  this->SetNumberOfRequiredInputs(2);

  // Create the filters
  m_WarpFilter = WarpFilterType::New();
  typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
  m_WarpFilter->SetInterpolator(interpolator);
  m_ExtractFilter = ExtractFilterType::New();
  m_MVFInterpolatorFilter = MVFInterpolatorType::New();
  m_PasteFilter = PasteFilterType::New();
  m_CastFilter = CastFilterType::New();
  m_ConstantSource = ConstantImageSourceType::New();

  // Set permanent connections
  m_WarpFilter->SetInput(m_ExtractFilter->GetOutput());
  m_WarpFilter->SetDisplacementField( m_MVFInterpolatorFilter->GetOutput() );
  m_CastFilter->SetInput(m_WarpFilter->GetOutput());
  m_PasteFilter->SetSourceImage(m_CastFilter->GetOutput());

  // Set permanent parameters
  m_ExtractFilter->SetDirectionCollapseToIdentity();

  // Set memory management parameters
}

template< typename TImageSequence, typename TMVFImageSequence, typename TImage, typename TMVFImage>
void
WarpSequenceImageFilter< TImageSequence, TMVFImageSequence, TImage, TMVFImage>
::SetDisplacementField(const TMVFImageSequence* MVFs)
{
  this->SetNthInput(1, const_cast<TMVFImageSequence*>(MVFs));
}

template< typename TImageSequence, typename TMVFImageSequence, typename TImage, typename TMVFImage>
typename TMVFImageSequence::Pointer
WarpSequenceImageFilter< TImageSequence, TMVFImageSequence, TImage, TMVFImage>
::GetDisplacementField()
{
  return static_cast< TMVFImageSequence * >
          ( this->itk::ProcessObject::GetInput(1) );
}

template< typename TImageSequence, typename TMVFImageSequence, typename TImage, typename TMVFImage>
void
WarpSequenceImageFilter< TImageSequence, TMVFImageSequence, TImage, TMVFImage>
::GenerateOutputInformation()
{
  int Dimension = TImageSequence::ImageDimension;

  // Set runtime connections
  m_ExtractFilter->SetInput(this->GetInput(0));
  m_MVFInterpolatorFilter->SetInput(this->GetDisplacementField());

  // Generate a signal and pass it to the MVFInterpolator
  std::vector<double> signal;
  int nbFrames = this->GetInput(0)->GetLargestPossibleRegion().GetSize(Dimension - 1);
  for (int frame = 0; frame < nbFrames; frame ++)
    {
    signal.push_back((float) frame / (float) nbFrames);
    }
  m_MVFInterpolatorFilter->SetSignalVector(signal);

  // Initialize the source
  m_ConstantSource->SetInformationFromImage(this->GetInput(0));
  m_ConstantSource->Update();
  m_PasteFilter->SetDestinationImage(m_ConstantSource->GetOutput());

  // Set extraction regions and indices
  m_ExtractAndPasteRegion = this->GetInput(0)->GetLargestPossibleRegion();
  m_ExtractAndPasteRegion.SetSize(Dimension - 1, 0);
  m_ExtractAndPasteRegion.SetIndex(Dimension - 1, 0);

  m_ExtractFilter->SetExtractionRegion(m_ExtractAndPasteRegion);
  m_MVFInterpolatorFilter->SetFrame(0);

  m_WarpFilter->SetOutputSpacing(m_ExtractFilter->GetOutput()->GetSpacing());
  m_WarpFilter->SetOutputOrigin(m_ExtractFilter->GetOutput()->GetOrigin());

  m_ExtractFilter->UpdateOutputInformation();
  m_MVFInterpolatorFilter->UpdateOutputInformation();
  m_CastFilter->UpdateOutputInformation();

  m_PasteFilter->SetSourceRegion(m_CastFilter->GetOutput()->GetLargestPossibleRegion());
  m_PasteFilter->SetDestinationIndex(m_ExtractAndPasteRegion.GetIndex());

  // Have the last filter calculate its output information
  m_PasteFilter->UpdateOutputInformation();

  // Copy it as the output information of the composite filter
  this->GetOutput()->CopyInformation( m_PasteFilter->GetOutput() );
}


template< typename TImageSequence, typename TMVFImageSequence, typename TImage, typename TMVFImage>
void
WarpSequenceImageFilter< TImageSequence, TMVFImageSequence, TImage, TMVFImage>
::GenerateInputRequestedRegion()
{
  //Call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  //Get pointers to the input and output
  typename TImageSequence::Pointer  inputPtr  = const_cast<TImageSequence *>(this->GetInput(0));
  inputPtr->SetRequestedRegionToLargestPossibleRegion();

  typename TMVFImageSequence::Pointer  inputMVFPtr  = this->GetDisplacementField();
  inputMVFPtr->SetRequestedRegionToLargestPossibleRegion();
}

template< typename TImageSequence, typename TMVFImageSequence, typename TImage, typename TMVFImage>
void
WarpSequenceImageFilter< TImageSequence, TMVFImageSequence, TImage, TMVFImage>
::GenerateData()
{
  int Dimension = TImageSequence::ImageDimension;

  // Declare an image pointer to disconnect the output of paste
  typename TImageSequence::Pointer pimg;

  for (int frame=0; frame<this->GetInput(0)->GetLargestPossibleRegion().GetSize(Dimension-1); frame++)
    {
    if (frame > 0) // After the first frame, use the output of paste as input
      {
      pimg = m_PasteFilter->GetOutput();
      pimg->DisconnectPipeline();
      m_PasteFilter->SetDestinationImage(pimg);
      }

    m_ExtractAndPasteRegion.SetIndex(Dimension - 1, frame);

    m_ExtractFilter->SetExtractionRegion(m_ExtractAndPasteRegion);
    m_MVFInterpolatorFilter->SetFrame(frame);

    m_CastFilter->Update();

    m_PasteFilter->SetSourceRegion(m_CastFilter->GetOutput()->GetLargestPossibleRegion());
    m_PasteFilter->SetDestinationIndex(m_ExtractAndPasteRegion.GetIndex());

    m_PasteFilter->Update();
    }
  this->GraftOutput( m_PasteFilter->GetOutput() );
}


}// end namespace


#endif