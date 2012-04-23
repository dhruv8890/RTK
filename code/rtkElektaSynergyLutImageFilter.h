#ifndef __rtkElektaSynergyLutImageFilter_h
#define __rtkElektaSynergyLutImageFilter_h

#include "rtkLutImageFilter.h"
#include <itkNumericTraits.h>

namespace rtk
{

/** \class ElektaSynergyLutImageFilter
 * \brief Lookup table for Elekta Synergy data.
 *
 * The lookup table converts the raw values to the logarithm of the value divided by the max
 *
 */
template <class TInputImage, class TOutputImage>
class ITK_EXPORT ElektaSynergyLutImageFilter : public LutImageFilter<TInputImage, TOutputImage>
{

public:
  /** Standard class typedefs. */
  typedef ElektaSynergyLutImageFilter               Self;
  typedef LutImageFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                   Pointer;
  typedef itk::SmartPointer<const Self>             ConstPointer;

  typedef typename TInputImage::PixelType           InputImagePixelType;
  typedef typename TOutputImage::PixelType          OutputImagePixelType;
  typedef typename Superclass::FunctorType::LutType LutType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(ElektaSynergyLutImageFilter, LutImageFilter);

protected:
  ElektaSynergyLutImageFilter();
  virtual ~ElektaSynergyLutImageFilter() {
  }

private:
  ElektaSynergyLutImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&);              //purposely not implemented

};

} // end namespace rtk

template <class TInputImage, class TOutputImage>
rtk::ElektaSynergyLutImageFilter<TInputImage, TOutputImage>::ElektaSynergyLutImageFilter()
{
  // Create the lut
  typename LutType::Pointer lut = LutType::New();
  typename LutType::SizeType size;
  size[0] = itk::NumericTraits<InputImagePixelType>::max()-itk::NumericTraits<InputImagePixelType>::min()+1;
  lut->SetRegions( size );
  lut->Allocate();

  // Iterate and set lut
  OutputImagePixelType                       logRef = log(OutputImagePixelType(size[0]) );
  itk::ImageRegionIteratorWithIndex<LutType> it( lut, lut->GetBufferedRegion() );
  it.GoToBegin();

  //First value takes value of pixel #1
  it.Set( logRef - log( OutputImagePixelType(size[0]-1) ) );
  ++it;

  //Conventional lookup table for the rest
  while( !it.IsAtEnd() ) {
    it.Set( logRef - log( OutputImagePixelType(size[0]-it.GetIndex()[0]) ) );
    ++it;
    }

  //Last value takes value of pixel #1
  --it;
  it.Set( logRef - log( OutputImagePixelType(size[0]-1) ) );
  ++it;

  // Set the lut to member and functor
  this->SetLut(lut);
}

#endif