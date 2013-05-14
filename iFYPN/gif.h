//
//  gif.h
//  OSXGLEssentials
//
//  Created by Charles Tangora on 8/22/12.
//
//

#ifndef gif_h
#define gif_h

#include <stdio.h>
#include <stdint.h>

#include <map>
#include <vector>
#include <algorithm>
// typedef std::map<uint32_t, std::pair<uint32_t, uint32_t> > Palette;

// Stores a color palette and also
// the 32-bit -> 8-bit mapping decisions that have been made
// as part of the palette's creation
// Wraps STL classes, I hope someday to remove the STL dependency
struct Palette
{
    std::vector<uint32_t> m_colors;
    std::map<uint32_t, uint32_t > m_count;
    std::map<uint32_t, uint32_t > m_pal;
    
    void IncCount( uint8_t r, uint8_t g, uint8_t b )
    {
        uint32_t rgb = (r << 16) | (g << 8) | b;
        if(m_count.find(rgb) != m_count.end())
            m_count[rgb]++;
        else
        {
            m_colors.push_back(rgb);
            m_count[rgb] = 1;
        }
    }
    
    void GetLinearPalette( uint32_t* palette ) const
    {
        bzero(palette, sizeof(uint32_t)*256);
        
        for(std::map<uint32_t, uint32_t >::const_iterator it = m_pal.begin(); it != m_pal.end(); ++it)
        {
            uint32_t entry = it->second;
            
            palette[entry >> 24] = entry;
        }
    }
    
    uint32_t GetPaletteEntry( uint8_t r, uint8_t g, uint8_t b )
    {
        uint32_t orgb = (r << 16) | (g << 8) | b;
        return m_pal[orgb];
    }
    
    void AssignColor( uint8_t r, uint8_t g, uint8_t b, uint8_t paletteEntry, uint8_t pal_r, uint8_t pal_g, uint8_t pal_b )
    {
        uint32_t orgb = (r << 16) | (g << 8) | b;
        uint32_t prgb = (paletteEntry << 24) | (pal_r << 16) | (pal_g << 8) | pal_b;
        
        m_pal[orgb] = prgb;
        m_pal[prgb & 0x00ffffff] = prgb;
    }
    
    void Clear()
    {
        m_pal.clear();
        m_colors.clear();
        m_count.clear();
    }
};

uint8_t Palettize( uint8_t* r, uint8_t* g, uint8_t* b, Palette& pal )
{
    uint32_t prgb = pal.GetPaletteEntry(*r, *g, *b);

    *r = (prgb >> 16) & 0xff;
    *g = (prgb >> 8) & 0xff;
    *b = (prgb) & 0xff;
    return (prgb >> 24) & 0xff;
}

void AssignColors( Palette* pPal, uint32_t* rPal, uint32_t* gPal, uint32_t* bPal )
{
    std::vector<uint32_t>& colors = pPal->m_colors;
    
    for (int ii=0; ii<colors.size(); ++ii)
    {
        uint32_t rgb = colors[ii];
        uint32_t r = rgb >> 16;
        uint32_t g = (rgb >> 8) & 0xff;
        uint32_t b = rgb & 0xff;
        
        uint32_t bestDiff = 10000;
        uint32_t bestInd = 256;
        for( uint32_t jj=0; jj<256; ++jj )
        {
            uint32_t diff = abs((int32_t)r-(int32_t)rPal[jj]) + abs((int32_t)g-(int32_t)gPal[jj]) + abs((int32_t)b-(int32_t)bPal[jj]);
            if( diff < bestDiff )
            {
                bestDiff = diff;
                bestInd = jj;
            }
        }
        
        pPal->AssignColor(r, g, b, bestInd, rPal[bestInd], gPal[bestInd], bPal[bestInd]);
    }
}

void FillInitialPalette( uint8_t* image, uint32_t width, uint32_t height, Palette* pPal )
{
    pPal->Clear();
    
    uint32_t numPixels = width*height;
    for( uint32_t ii=0; ii<numPixels; ++ii )
    {
        uint8_t r = image[ii*4];
        uint8_t g = image[ii*4+1];
        uint8_t b = image[ii*4+2];
        
        pPal->IncCount(r,g,b);
    }
}

void KMeansIterate( uint8_t* image, uint32_t width, uint32_t height, Palette* pPal )
{
    uint32_t rAvg[256];
    uint32_t gAvg[256];
    uint32_t bAvg[256];
    
    uint32_t count[256];
    
    bzero(rAvg, sizeof(count));
    bzero(gAvg, sizeof(count));
    bzero(bAvg, sizeof(count));
    bzero(count, sizeof(count));
    
    const std::vector<uint32_t>& colors = pPal->m_colors;
    
    uint32_t maxR_r = 0, maxR_g = 0, maxR_b = 0;
    uint32_t maxG_r = 0, maxG_g = 0, maxG_b = 0;
    uint32_t maxB_r = 0, maxB_g = 0, maxB_b = 0;
    
    for (uint32_t ii=0; ii<colors.size(); ++ii)
    {
        uint32_t rgb = colors[ii];
        uint32_t r = rgb >> 16;
        uint32_t g = (rgb >> 8) & 0xff;
        uint32_t b = rgb & 0xff;
        
        uint32_t palCount = pPal->m_count.find(rgb)->second;
        uint32_t p = pPal->m_pal.find(rgb)->second >> 24;
        
        if(r > maxR_r) { maxR_r = r; maxR_g = g; maxR_b = b; }
        if(g > maxG_g) { maxG_r = r; maxG_g = g; maxG_b = b; }
        if(b > maxG_b) { maxB_r = r; maxB_g = g; maxB_b = b; }
        
        rAvg[p] += r * palCount;
        gAvg[p] += g * palCount;
        bAvg[p] += b * palCount;
        count[p] += palCount;
    }
    
    for( uint32_t ii=2; ii<255; ++ii )
    {
        if( count[ii] > 0 )
        {
            uint32_t bias = count[ii]/2;
            
            rAvg[ii] += bias;
            gAvg[ii] += bias;
            bAvg[ii] += bias;
            
            rAvg[ii] /= count[ii];
            gAvg[ii] /= count[ii];
            bAvg[ii] /= count[ii];
        }
    }
    
    rAvg[0] = gAvg[0] = bAvg[0] = 0;
    rAvg[1] = gAvg[1] = bAvg[1] = 0;
    
    rAvg[252] = maxR_r; gAvg[252] = maxR_g; bAvg[252] = maxR_b;
    rAvg[253] = maxG_r; gAvg[253] = maxG_g; bAvg[253] = maxG_b;
    rAvg[254] = maxB_r; gAvg[254] = maxB_g; bAvg[254] = maxB_b;
    rAvg[255] = gAvg[255] = bAvg[255] = 255;
    
    std::vector< std::pair<uint64_t, uint32_t> > paletteDists;
    for (uint32_t ii=0; ii<colors.size(); ++ii)
    {
        uint32_t orgb = colors[ii];
        int32_t oor = orgb >> 16;
        int32_t og = (orgb >> 8) & 0xff;
        int32_t ob = orgb & 0xff;
        
        uint32_t prgb = pPal->m_pal[orgb];
        int32_t pr = prgb >> 16;
        int32_t pg = (prgb >> 8) & 0xff;
        int32_t pb = prgb & 0xff;
        
        uint32_t pcount = pPal->m_count[orgb];
        uint64_t dist = (abs(oor-pr) + abs(og-pg) + abs(ob-pb)) * pcount;
        
        paletteDists.push_back(std::pair<uint64_t, uint32_t>(dist, orgb));
    }
    std::sort(paletteDists.begin(), paletteDists.end());
    
    for( uint32_t ii=2; ii<252; ++ii )
    {
        if( count[ii] == 0 && paletteDists.size() > 0 )
        {
            uint32_t col = paletteDists.back().second;
            paletteDists.pop_back();
            
            rAvg[ii] = col >> 16;
            gAvg[ii] = (col >> 8) & 0xff;
            bAvg[ii] = col & 0xff;
        }
    }
    
    AssignColors(pPal, rAvg, gAvg, bAvg);
}

void MakePalette( uint8_t* image, uint32_t width, uint32_t height, Palette* pPal )
{
    FillInitialPalette(image, width, height, pPal);
    
    uint32_t rBas[256];
    uint32_t gBas[256];
    uint32_t bBas[256];
    
    rBas[0] = gBas[0] = bBas[0] = 0;
    rBas[1] = gBas[1] = bBas[1] = 0;
    rBas[255] = gBas[255] = bBas[255] = 255;
    
    for( uint32_t ii=2; ii<255; ++ii )
    {
        uint32_t x = rand() % width;
        uint32_t y = rand() % height;
        uint8_t r = image[(y*width+x)*4];
        uint8_t g = image[(y*width+x)*4+1];
        uint8_t b = image[(y*width+x)*4+2];
        rBas[ii] = r;
        gBas[ii] = g;
        bBas[ii] = b;
    }
    
    AssignColors(pPal, rBas, gBas, bBas);
    
    for(uint32_t ii=0; ii<5; ++ii)
    {
        KMeansIterate(image, width, height, pPal);
    }
}

void GetLinearPalette( const Palette* pPal, uint32_t* rLst, uint32_t* gLst, uint32_t* bLst )
{
    bzero(rLst, sizeof(uint32_t)*256);
    bzero(gLst, sizeof(uint32_t)*256);
    bzero(bLst, sizeof(uint32_t)*256);
    
    const std::vector<uint32_t>& colors = pPal->m_colors;
    
    for (int ii=0; ii<colors.size(); ++ii)
    {
        uint32_t prgb = pPal->m_pal.find(colors[ii])->second;
        uint32_t p = (prgb >> 24) & 0xff;
        uint32_t r = (prgb >> 16) & 0xff;
        uint32_t g = (prgb >> 8) & 0xff;
        uint32_t b = (prgb) & 0xff;
        
        rLst[p] = r;
        gLst[p] = g;
        bLst[p] = b;
    }
    
    rLst[0] = gLst[0] = bLst[0] = 0;
    rLst[1] = gLst[1] = bLst[1] = 0;
    rLst[255] = gLst[255] = bLst[255] = 255;
}

void TransferPalette( Palette* pPal, uint8_t* image, uint32_t width, uint32_t height )
{
    uint32_t rOld[256];
    uint32_t gOld[256];
    uint32_t bOld[256];
    
    GetLinearPalette(pPal, rOld, gOld, bOld);
    
    FillInitialPalette(image, width, height, pPal);
    AssignColors(pPal, rOld, gOld, bOld);
    
    KMeansIterate(image, width, height, pPal);
}

struct BitStatus
{
    uint8_t bitIndex;
    uint8_t byte;
    
    uint32_t chunkIndex;
    uint8_t chunk[256];
};

void WriteBit( BitStatus& stat, uint32_t bit )
{
    bit = bit & 1;
    bit = bit << stat.bitIndex;
    stat.byte |= bit;
    
    ++stat.bitIndex;
    if( stat.bitIndex > 7 )
    {
        stat.chunk[stat.chunkIndex++] = stat.byte;
        stat.bitIndex = 0;
        stat.byte = 0;
    }
}

void WriteImageChunk( FILE* f, BitStatus& stat )
{
    ASSERT(stat.bitIndex == 0);
    ASSERT(stat.chunkIndex < 256);
    
    fputc(stat.chunkIndex, f);
    fwrite(stat.chunk, 1, stat.chunkIndex, f);
    
    stat.bitIndex = 0;
    stat.byte = 0;
    stat.chunkIndex = 0;
}

void WriteCode( FILE* f, BitStatus& stat, uint32_t code, uint32_t length )
{
    for( uint32_t ii=0; ii<length; ++ii )
    {
        WriteBit(stat, code);
        code = code >> 1;
        
        if( stat.chunkIndex == 255 )
        {
            WriteImageChunk(f, stat);
        }
    }
}

struct lzwNode
{
    uint32_t m_code;
    lzwNode* m_children[256];
};

lzwNode* InitLzwTree()
{
    lzwNode* root = (lzwNode*)malloc(sizeof(lzwNode));
    for(int ii=0; ii<256; ++ii)
    {
        lzwNode* child = (lzwNode*)malloc(sizeof(lzwNode));
        bzero(child, sizeof(lzwNode));
        child->m_code = ii;
        root->m_children[ii] = child;
    }
    return root;
}

void DeleteLzwTree( lzwNode* node )
{
    if( !node ) return;
    
    for( uint32_t ii=0; ii<256; ++ii )
        DeleteLzwTree(node->m_children[ii]);
    
    free(node);
}

void WritePalette( const Palette* pPal, FILE* f )
{
    uint32_t cols[256];
    
    pPal->GetLinearPalette(cols);
    
    fputc(0, f);  // first and second colors: always black
    fputc(0, f);
    fputc(0, f);
    
    fputc(0, f);
    fputc(0, f);
    fputc(0, f);
    
    for(uint32_t ii=2; ii<255; ++ii)
    {
        uint32_t r = (cols[ii] >> 16) & 0xff;
        uint32_t g = (cols[ii] >> 8) & 0xff;
        uint32_t b = (cols[ii]) & 0xff;
        
        fputc(r, f);
        fputc(g, f);
        fputc(b, f);
    }
    
    fputc(255, f);  // last color: always white
    fputc(255, f);
    fputc(255, f);
}

void WriteLzwImage(FILE* f, uint8_t* image, uint8_t* oldImage, uint32_t left, uint32_t top,  uint32_t width, uint32_t height, uint32_t delay, Palette& pal)
{
    // graphics control extension
    fputc(0x21, f);
    fputc(0xf9, f);
    fputc(0x04, f);
    fputc(0x05, f); // dispose by leaving in place, has transparency
    fputc(delay & 0xff, f);
    fputc((delay >> 8) & 0xff, f);
    fputc(0x01, f); // transparent color index 1
    fputc(0, f);
    
    fputc(0x2c, f); // image descriptor block
    
    fputc(left & 0xff, f);           // corner of image in canvas space
    fputc((left >> 8) & 0xff, f);
    fputc(top & 0xff, f);
    fputc((top >> 8) & 0xff, f);
    
    fputc(width & 0xff, f);          // width and height of image
    fputc((width >> 8) & 0xff, f);
    fputc(height & 0xff, f);
    fputc((height >> 8) & 0xff, f);
    
    //fputc(0, f); // no local color table, no transparency
    //fputc(0x80, f); // no local color table, but transparency
    
    fputc(0x87, f); // local color table present, 256 entries
    WritePalette(&pal, f);
    
    fputc(8, f); // min code size 8 bits
    
    lzwNode* codetree = InitLzwTree();
    lzwNode* curNode = codetree;
    uint32_t codeSize = 9;
    uint32_t maxCode = 257;
    
    BitStatus stat;
    stat.byte = 0;
    stat.bitIndex = 0;
    stat.chunkIndex = 0;
    
    WriteCode(f, stat, 256, codeSize);
    
    for(int32_t yy=0; yy<height; ++yy)
    {
        for(int32_t xx=0; xx<width; ++xx)
        {
            uint8_t nextValue = image[(yy*width+xx)*4+3];
            
            // "loser mode" - no compression, every single code is followed immediately by a clear
            //WriteCode( f, stat, nextValue, codeSize );
            //WriteCode( f, stat, 256, codeSize );
            
            if( curNode->m_children[nextValue] )
            {
                curNode = curNode->m_children[nextValue];
            }
            else
            {
                WriteCode( f, stat, curNode->m_code, codeSize );
                
                lzwNode* child = (lzwNode*)malloc(sizeof(lzwNode));
                bzero(child, sizeof(lzwNode));
                maxCode++;
                child->m_code = maxCode;
                curNode->m_children[nextValue] = child;
                
                if( maxCode >= (1 << codeSize) )
                {
                    codeSize++;
                }
                if( maxCode == 4095 )
                {
                    ASSERT(codeSize == 12);
                    WriteCode(f, stat, 256, codeSize); // clear tree
                    
                    DeleteLzwTree(codetree);
                    codetree = InitLzwTree();
                    curNode = codetree;
                    codeSize = 9;
                    maxCode = 257;
                }
                
                curNode = codetree->m_children[nextValue];
            }
        }
    }
    
    WriteCode( f, stat, curNode->m_code, codeSize );
    WriteCode( f, stat, 256, codeSize );
    WriteCode( f, stat, 257, 9 );
    while( stat.bitIndex ) WriteBit(stat, 0);
    if( stat.chunkIndex ) WriteImageChunk(f, stat);
    DeleteLzwTree(codetree);
    fputc(0, f); // image block terminator
}

void SetTransparency( uint8_t* lastFrame, uint8_t* nextFrame, uint32_t width, uint32_t height )
{
    uint32_t numPixels = width*height;
    for( uint32_t ii=0; ii<numPixels; ++ii )
    {
        if( lastFrame[0] == nextFrame[0] &&
            lastFrame[1] == nextFrame[1] &&
            lastFrame[2] == nextFrame[2] )
        {
            // nextFrame[0] = nextFrame[1] = nextFrame[2] = nextFrame[3] = 0;
            nextFrame[3] = 0;
        }
        else
        {
            nextFrame[3] = 255;
        }
        
        lastFrame += 4;
        nextFrame += 4;
    }
}

void DitherImage( uint8_t* lastFrame, uint8_t* nextFrame, uint32_t width, uint32_t height, Palette& pal )
{
    uint32_t rPal[256];
    uint32_t gPal[256];
    uint32_t bPal[256];
    GetLinearPalette(&pal, rPal, gPal, bPal);
    
    uint32_t numPixels = width*height;
    int32_t* quantPixels = (int32_t*)malloc(sizeof(int32_t)*numPixels*4);
    
    for( uint32_t ii=0; ii<numPixels*4; ++ii )
    {
        uint8_t pix = nextFrame[ii];
        uint16_t pix16 = (uint16_t)(pix) << 8;
        quantPixels[ii] = pix16;
    }
    
    for( uint32_t yy=0; yy<height; ++yy )
    {
        for( uint32_t xx=0; xx<width; ++xx )
        {
            int32_t* nextPix = quantPixels + 4*(yy*width+xx);
            uint8_t* lastPix = lastFrame? lastFrame + 4*(yy*width+xx) : NULL;
            
            uint8_t rr = nextPix[0] >> 8;
            uint8_t gg = nextPix[1] >> 8;
            uint8_t bb = nextPix[2] >> 8;
            
            if( lastFrame &&
                lastPix[0] == rr &&
                lastPix[1] == gg &&
                lastPix[2] == bb )
            {
                nextPix[0] = rr;
                nextPix[1] = gg;
                nextPix[2] = bb;
                nextPix[3] = 1;
                continue;
            }
            
            int32_t bestDiff = 1000000;
            int32_t bestInd = 256;
            
            int32_t r_err;
            int32_t g_err;
            int32_t b_err;

            for( uint32_t jj=0; jj<256; ++jj )
            {
                int32_t r_ierr = ((int32_t)nextPix[0]) - (((int32_t)rPal[jj]) << 8);
                int32_t g_ierr = ((int32_t)nextPix[1]) - (((int32_t)gPal[jj]) << 8);
                int32_t b_ierr = ((int32_t)nextPix[2]) - (((int32_t)bPal[jj]) << 8);
                
                int32_t diff = abs(r_ierr)+abs(g_ierr)+abs(b_ierr);
                if( diff < bestDiff )
                {
                    bestDiff = diff;
                    bestInd = jj;
                    r_err = r_ierr;
                    g_err = g_ierr;
                    b_err = b_ierr;
                }
            }
            
            nextPix[0] = rPal[bestInd];
            nextPix[1] = gPal[bestInd];
            nextPix[2] = bPal[bestInd];
            nextPix[3] = bestInd;
            
            int quantloc_7 = (yy*width+xx+1);
            int quantloc_3 = (yy*width+width+xx-1);
            int quantloc_5 = (yy*width+width+xx);
            int quantloc_1 = (yy*width+width+xx+1);
            
            if(quantloc_7 < numPixels)
            {
                int32_t* pix7 = quantPixels+4*quantloc_7;
                pix7[0] += r_err * 7 / 16;
                pix7[1] += g_err * 7 / 16;
                pix7[2] += b_err * 7 / 16;
            }
            
            if(quantloc_3 < numPixels)
            {
                int32_t* pix3 = quantPixels+4*quantloc_3;
                pix3[0] += r_err * 3 / 16;
                pix3[1] += g_err * 3 / 16;
                pix3[2] += b_err * 3 / 16;
            }
            
            if(quantloc_5 < numPixels)
            {
                int32_t* pix5 = quantPixels+4*quantloc_5;
                pix5[0] += r_err * 5 / 16;
                pix5[1] += g_err * 5 / 16;
                pix5[2] += b_err * 5 / 16;
            }
            
            if(quantloc_1 < numPixels)
            {
                int32_t* pix1 = quantPixels+4*quantloc_1;
                pix1[0] += r_err / 16;
                pix1[1] += g_err / 16;
                pix1[2] += b_err / 16;
            }
        }
    }
    
    for( uint32_t ii=0; ii<numPixels*4; ++ii )
    {
        nextFrame[ii] = quantPixels[ii];
        if(lastFrame)
            lastFrame[ii] = quantPixels[ii];
    }
    
    free(quantPixels);
}

void WritePaletteResultsToImage( uint8_t* lastFrame, uint8_t* nextFrame, uint32_t width, uint32_t height, Palette& pal )
{
    uint32_t numPixels = width*height;
    for( uint32_t ii=0; ii<numPixels; ++ii )
    {
        uint8_t a = nextFrame[3];
        
        if( a )
        {        
            uint8_t r = nextFrame[0];
            uint8_t g = nextFrame[1];
            uint8_t b = nextFrame[2];
            uint8_t p = Palettize(&r, &g, &b, pal);
        
            nextFrame[3] = p;
            
            lastFrame[0] = r;
            lastFrame[1] = g;
            lastFrame[2] = b;
        }
        else
        {
            nextFrame[3] = 1;
        }
        
        lastFrame += 4;
        nextFrame += 4;
    }
}

struct GifWriter
{
    FILE* f;
    Palette pal;
    uint8_t* oldImage;
};

void BeginGif( GifWriter* writer, const char* filename, uint8_t* image, uint32_t width, uint32_t height, uint32_t delay )
{
    writer->f = fopen(filename, "wb");
    ALWAYS_ASSERT(writer->f);
    writer->oldImage = (uint8_t*)malloc(width*height*4);
    memcpy(writer->oldImage, image, width*height*4);
    
    MakePalette(image, width, height, &writer->pal);
    
    //DitherImage(NULL, image, width, height, writer->pal);
    WritePaletteResultsToImage(writer->oldImage, image, width, height, writer->pal);
    
    fputs("GIF89a", writer->f);
    
    // screen descriptor
    fputc(width & 0xff, writer->f);
    fputc((width >> 8) & 0xff, writer->f);
    fputc(height & 0xff, writer->f);
    fputc((height >> 8) & 0xff, writer->f);
    
    fputc(0xf7, writer->f);  // there is an unsorted global color table of 256 entries
    fputc(0, writer->f);     // background color
    fputc(0, writer->f);     // pixels are square (we need to specify this because it's 1989)
    
    // now the global palette
    WritePalette(&writer->pal, writer->f);
    
    if( delay != 0 )
    {
        // animation header
        fputc(0x21, writer->f); // extension
        fputc(0xff, writer->f); // application specific
        fputc(11, writer->f); // length 11
        fputs("NETSCAPE2.0", writer->f); // yes, really
        fputc(3, writer->f); // 3 bytes of NETSCAPE2.0 data
        
        fputc(1, writer->f); // JUST BECAUSE
        fputc(0, writer->f); // loop infinitely (byte 0)
        fputc(0, writer->f); // loop infinitely (byte 1)
        
        fputc(0, writer->f); // block terminator
    }
    
    WriteLzwImage(writer->f, image, writer->oldImage, 0, 0, width, height, delay, writer->pal);
}

void ContinueGif( GifWriter* writer, uint8_t* image, uint32_t width, uint32_t height, uint32_t delay )
{
    SetTransparency(writer->oldImage, image, width, height);
    TransferPalette(&writer->pal, image, width, height);
    
    DitherImage(writer->oldImage, image, width, height, writer->pal);
    //WritePaletteResultsToImage(writer->oldImage, image, width, height, writer->pal);
    
    WriteLzwImage(writer->f, image, writer->oldImage, 0, 0, width, height, delay, writer->pal);
}

void EndGif( GifWriter* writer )
{
    fputc(0x3b, writer->f); // end of file
    fclose(writer->f);
    free(writer->oldImage);
    
    writer->f = NULL;
    writer->oldImage = NULL;
}

#endif
