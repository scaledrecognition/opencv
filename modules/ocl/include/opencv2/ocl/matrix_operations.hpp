/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2010-2012, Institute Of Software Chinese Academy Of Science, all rights reserved.
// Copyright (C) 2010-2012, Advanced Micro Devices, Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other oclMaterials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef __OPENCV_GPU_MATRIX_OPERATIONS_HPP__
#define __OPENCV_GPU_MATRIX_OPERATIONS_HPP__

namespace cv
{

    namespace ocl
    {
        ////////////////////////////////////OpenCL kernel strings//////////////////////////
        //extern const char *convertC3C4;

        ////////////////////////////////////////////////////////////////////////
        //////////////////////////////// oclMat ////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        inline oclMat::oclMat() : flags(0), rows(0), cols(0), step(0), data(0), refcount(0), datastart(0), dataend(0), offset(0), wholerows(0), wholecols(0) {}

        inline oclMat::oclMat(int _rows, int _cols, int _type) : flags(0), rows(0), cols(0), step(0), data(0), refcount(0), datastart(0), dataend(0), offset(0), wholerows(0), wholecols(0)
        {
            if( _rows > 0 && _cols > 0 )
                create( _rows, _cols, _type );
        }

        inline oclMat::oclMat(Size _size, int _type) : flags(0), rows(0), cols(0), step(0), data(0), refcount(0), datastart(0), dataend(0), offset(0), wholerows(0), wholecols(0)
        {
            if( _size.height > 0 && _size.width > 0 )
                create( _size.height, _size.width, _type );
        }

        inline oclMat::oclMat(int _rows, int _cols, int _type, const Scalar &_s)
            : flags(0), rows(0), cols(0), step(0), data(0), refcount(0), datastart(0), dataend(0), offset(0), wholerows(0), wholecols(0)
        {
            if(_rows > 0 && _cols > 0)
            {
                create(_rows, _cols, _type);
                *this = _s;
            }
        }

        inline oclMat::oclMat(Size _size, int _type, const Scalar &_s)
            : flags(0), rows(0), cols(0), step(0), data(0), refcount(0), datastart(0), dataend(0), offset(0), wholerows(0), wholecols(0)
        {
            if( _size.height > 0 && _size.width > 0 )
            {
                create( _size.height, _size.width, _type );
                *this = _s;
            }
        }

        inline oclMat::oclMat(const oclMat &m)
            : flags(m.flags), rows(m.rows), cols(m.cols), step(m.step), data(m.data),
              refcount(m.refcount), datastart(m.datastart), dataend(m.dataend), clCxt(m.clCxt), offset(m.offset), wholerows(m.wholerows), wholecols(m.wholecols)
        {
            if( refcount )
                CV_XADD(refcount, 1);
        }
        //Fixme, the data is not correct if _data point to the CPU memory
        inline oclMat::oclMat(int _rows, int _cols, int _type, void *_data, size_t _step)
            : flags(Mat::MAGIC_VAL + (_type &TYPE_MASK)), rows(_rows), cols(_cols), step(_step), data((uchar *)_data), refcount(0),
              datastart((uchar *)_data), dataend((uchar *)_data), offset(0), wholerows(_rows), wholecols(_cols)
        {
            size_t minstep = cols * elemSize();
            if( step == Mat::AUTO_STEP )
            {
                step = minstep;
                flags |= Mat::CONTINUOUS_FLAG;
            }
            else
            {
                if( rows == 1 ) step = minstep;
                CV_DbgAssert( step >= minstep );
                flags |= step == minstep ? Mat::CONTINUOUS_FLAG : 0;
            }
            dataend += step * (rows - 1) + minstep;
        }
        //Fixme, the data is not correct if _data point to the CPU memory
        inline oclMat::oclMat(Size _size, int _type, void *_data, size_t _step)
            : flags(Mat::MAGIC_VAL + (_type &TYPE_MASK)), rows(_size.height), cols(_size.width),
              step(_step), data((uchar *)_data), refcount(0),
              datastart((uchar *)_data), dataend((uchar *)_data), offset(0), wholerows(_size.height), wholecols(_size.width)
        {
            size_t minstep = cols * elemSize();
            if( step == Mat::AUTO_STEP )
            {
                step = minstep;
                flags |= Mat::CONTINUOUS_FLAG;
            }
            else
            {
                if( rows == 1 ) step = minstep;
                CV_DbgAssert( step >= minstep );
                flags |= step == minstep ? Mat::CONTINUOUS_FLAG : 0;
            }
            dataend += step * (rows - 1) + minstep;
        }


        inline oclMat::oclMat(const oclMat &m, const Range &rowRange, const Range &colRange)
        {
            flags = m.flags;
            step = m.step;
            refcount = m.refcount;
            data = m.data;
            datastart = m.datastart;
            dataend = m.dataend;
            wholerows = m.wholerows;
            wholecols = m.wholecols;
            offset = m.offset;
            if( rowRange == Range::all() )
                rows = m.rows;
            else
            {
                CV_Assert( 0 <= rowRange.start && rowRange.start <= rowRange.end && rowRange.end <= m.rows );
                rows = rowRange.size();
                offset += step * rowRange.start;
            }

            if( colRange == Range::all() )
                cols = m.cols;
            else
            {
                CV_Assert( 0 <= colRange.start && colRange.start <= colRange.end && colRange.end <= m.cols );
                cols = colRange.size();
                offset += colRange.start * elemSize();
                flags &= cols < m.cols ? ~Mat::CONTINUOUS_FLAG : -1;
            }

            if( rows == 1 )
                flags |= Mat::CONTINUOUS_FLAG;

            if( refcount )
                CV_XADD(refcount, 1);
            if( rows <= 0 || cols <= 0 )
                rows = cols = 0;
        }

        inline oclMat::oclMat(const oclMat &m, const Rect &roi)
            : flags(m.flags), rows(roi.height), cols(roi.width),
              step(m.step), data(m.data), refcount(m.refcount),
              datastart(m.datastart), dataend(m.dataend), clCxt(m.clCxt), offset(m.offset), wholerows(m.wholerows), wholecols(m.wholecols)
        {
            flags &= roi.width < m.cols ? ~Mat::CONTINUOUS_FLAG : -1;
            offset += roi.y * step + roi.x * elemSize();
            CV_Assert( 0 <= roi.x && 0 <= roi.width && roi.x + roi.width <= m.cols &&
                       0 <= roi.y && 0 <= roi.height && roi.y + roi.height <= m.rows );
            if( refcount )
                CV_XADD(refcount, 1);
            if( rows <= 0 || cols <= 0 )
                rows = cols = 0;
        }

        inline oclMat::oclMat(const Mat &m)
            : flags(0), rows(0), cols(0), step(0), data(0), refcount(0), datastart(0), dataend(0) , offset(0), wholerows(0), wholecols(0)
        {
            //clCxt = Context::getContext();
            upload(m);
        }

        inline oclMat::~oclMat()
        {
            release();
        }

        inline oclMat &oclMat::operator = (const oclMat &m)
        {
            if( this != &m )
            {
                if( m.refcount )
                    CV_XADD(m.refcount, 1);
                release();
                clCxt = m.clCxt;
                flags = m.flags;
                rows = m.rows;
                cols = m.cols;
                step = m.step;
                data = m.data;
                datastart = m.datastart;
                dataend = m.dataend;
                offset = m.offset;
                wholerows = m.wholerows;
                wholecols = m.wholecols;
                refcount = m.refcount;
            }
            return *this;
        }

        inline oclMat &oclMat::operator = (const Mat &m)
        {
            //clCxt = Context::getContext();
            upload(m);
            return *this;
        }

        /* Fixme! To be supported in OpenCL later. */
#if 0
        template <class T> inline oclMat::operator DevMem2D_<T>() const
        {
            return DevMem2D_<T>(rows, cols, (T *)data, step);
        }
        template <class T> inline oclMat::operator PtrStep_<T>() const
        {
            return PtrStep_<T>(static_cast< DevMem2D_<T> >(*this));
        }
#endif

        //CPP: void oclMat::upload(const Mat& m);

        inline oclMat::operator Mat() const
        {
            Mat m;
            download(m);
            return m;
        }

        //CPP void oclMat::download(cv::Mat& m) const;

        inline oclMat oclMat::row(int y) const
        {
            return oclMat(*this, Range(y, y + 1), Range::all());
        }
        inline oclMat oclMat::col(int x) const
        {
            return oclMat(*this, Range::all(), Range(x, x + 1));
        }
        inline oclMat oclMat::rowRange(int startrow, int endrow) const
        {
            return oclMat(*this, Range(startrow, endrow), Range::all());
        }
        inline oclMat oclMat::rowRange(const Range &r) const
        {
            return oclMat(*this, r, Range::all());
        }
        inline oclMat oclMat::colRange(int startcol, int endcol) const
        {
            return oclMat(*this, Range::all(), Range(startcol, endcol));
        }
        inline oclMat oclMat::colRange(const Range &r) const
        {
            return oclMat(*this, Range::all(), r);
        }

        inline oclMat oclMat::clone() const
        {
            oclMat m;
            copyTo(m);
            return m;
        }

        //CPP void oclMat::copyTo( oclMat& m ) const;
        //CPP void oclMat::copyTo( oclMat& m, const oclMat& mask  ) const;
        //CPP void oclMat::convertTo( oclMat& m, int rtype, double alpha=1, double beta=0 ) const;

        inline void oclMat::assignTo( oclMat &m, int type ) const
        {
            if( type < 0 )
                m = *this;
            else
                convertTo(m, type);
        }

        //CPP oclMat& oclMat::operator = (const Scalar& s);
        //CPP oclMat& oclMat::setTo(const Scalar& s, const oclMat& mask=oclMat());
        //CPP oclMat oclMat::reshape(int _cn, int _rows=0) const;
        inline void oclMat::create(Size _size, int _type)
        {
            create(_size.height, _size.width, _type);
        }
        //CPP void oclMat::create(int _rows, int _cols, int _type);
        //CPP void oclMat::release();

        inline void oclMat::swap(oclMat &b)
        {
            std::swap( flags, b.flags );
            std::swap( rows, b.rows );
            std::swap( cols, b.cols );
            std::swap( step, b.step );
            std::swap( data, b.data );
            std::swap( datastart, b.datastart );
            std::swap( dataend, b.dataend );
            std::swap( refcount, b.refcount );
            std::swap( offset, b.offset );
            std::swap( wholerows, b.wholerows );
            std::swap( wholecols, b.wholecols );
        }

        inline void oclMat::locateROI( Size &wholeSize, Point &ofs ) const
        {
            size_t esz = elemSize();//, minstep;
            //ptrdiff_t delta1 = offset;//, delta2 = dataend - datastart;
            CV_DbgAssert( step > 0 );
            if( offset == 0 )
                ofs.x = ofs.y = 0;
            else
            {
                ofs.y = (int)(offset / step);
                ofs.x = (int)((offset - step * ofs.y) / esz);
                //CV_DbgAssert( data == datastart + ofs.y*step + ofs.x*esz );
            }
            //minstep = (ofs.x + cols)*esz;
            //wholeSize.height = (int)((delta2 - minstep)/step + 1);
            //wholeSize.height = std::max(wholeSize.height, ofs.y + rows);
            //wholeSize.width = (int)((delta2 - step*(wholeSize.height-1))/esz);
            //wholeSize.width = std::max(wholeSize.width, ofs.x + cols);
            wholeSize.height = wholerows;
            wholeSize.width = wholecols;
        }

        inline oclMat &oclMat::adjustROI( int dtop, int dbottom, int dleft, int dright )
        {
            Size wholeSize;
            Point ofs;
            size_t esz = elemSize();
            locateROI( wholeSize, ofs );
            int row1 = std::max(ofs.y - dtop, 0), row2 = std::min(ofs.y + rows + dbottom, wholeSize.height);
            int col1 = std::max(ofs.x - dleft, 0), col2 = std::min(ofs.x + cols + dright, wholeSize.width);
            offset += (row1 - ofs.y) * step + (col1 - ofs.x) * esz;
            rows = row2 - row1;
            cols = col2 - col1;
            if( esz *cols == step || rows == 1 )
                flags |= Mat::CONTINUOUS_FLAG;
            else
                flags &= ~Mat::CONTINUOUS_FLAG;
            return *this;
        }

        inline oclMat oclMat::operator()( Range rowRange, Range colRange ) const
        {
            return oclMat(*this, rowRange, colRange);
        }
        inline oclMat oclMat::operator()( const Rect &roi ) const
        {
            return oclMat(*this, roi);
        }

        inline bool oclMat::isContinuous() const
        {
            return (flags & Mat::CONTINUOUS_FLAG) != 0;
        }
        inline size_t oclMat::elemSize() const
        {
            return CV_ELEM_SIZE(flags);
        }
        inline size_t oclMat::elemSize1() const
        {
            return CV_ELEM_SIZE1(flags);
        }
        inline int oclMat::type() const
        {
            return CV_MAT_TYPE(flags);
        }
        inline int oclMat::depth() const
        {
            return CV_MAT_DEPTH(flags);
        }
        inline int oclMat::channels() const
        {
            return CV_MAT_CN(flags);
        }
        inline size_t oclMat::step1() const
        {
            return step / elemSize1();
        }
        inline Size oclMat::size() const
        {
            return Size(cols, rows);
        }
        inline bool oclMat::empty() const
        {
            return data == 0;
        }


        //fixme, the ROI operation is not correct.
        inline uchar *oclMat::ptr(int y)
        {
            CV_DbgAssert( (unsigned)y < (unsigned)rows );
            return data + step * y;
        }

        inline const uchar *oclMat::ptr(int y) const
        {
            CV_DbgAssert( (unsigned)y < (unsigned)rows );
            return data + step * y;
        }

        template<typename _Tp> inline _Tp *oclMat::ptr(int y)
        {
            CV_DbgAssert( (unsigned)y < (unsigned)rows );
            return (_Tp *)(data + step * y);
        }

        template<typename _Tp> inline const _Tp *oclMat::ptr(int y) const
        {
            CV_DbgAssert( (unsigned)y < (unsigned)rows );
            return (const _Tp *)(data + step * y);
        }

        inline oclMat oclMat::t() const
        {
            oclMat tmp;
            transpose(*this, tmp);
            return tmp;
        }

        static inline void swap( oclMat &a, oclMat &b )
        {
            a.swap(b);
        }

    } /* end of namespace ocl */

} /* end of namespace cv */

#endif /* __OPENCV_GPU_MATRIX_OPERATIONS_HPP__ */
