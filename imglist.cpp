// File:        imglist.cpp
// Date:        2022-01-27 10:21
// Description: Contains partial implementation of ImgList class
//              for CPSC 221 2021W2 PA1
//              Function bodies to be completed by yourselves
//
// ADD YOUR FUNCTION IMPLEMENTATIONS IN THIS FILE
//

#include "imglist.h"

#include <math.h> // provides fmin and fabs functions

/**************************
* MISCELLANEOUS FUNCTIONS *
**************************/

/*
* This function is NOT part of the ImgList class,
* but will be useful for one of the ImgList functions.
* Returns the "difference" between two hue values.
* PRE: hue1 is a double between [0,360).
* PRE: hue2 is a double between [0,360).
* 
* The hue difference is the absolute difference between two hues,
* but takes into account differences spanning the 360 value.
* e.g. Two pixels with hues 90 and 110 differ by 20, but
*      two pixels with hues 5 and 355 differ by 10.
*/
double HueDiff(double hue1, double hue2) {
  return fmin(fabs(hue1 - hue2), fabs(hue2 + 360 - hue1));
}

/*********************
* CONSTRUCTORS, ETC. *
*********************/

/*
* Default constructor. Makes an empty list
*/
ImgList::ImgList() {
  // northwest = new ImgNode();
  // southeast = new ImgNode();
  northwest = nullptr;
  southeast = nullptr;
}

/*
* Creates a list from image data
* PRE: img has dimensions of at least 1x1
*/
ImgList::ImgList(PNG& img) {
  northwest = new ImgNode();
  ImgNode* head = northwest;
  ImgNode* prev = nullptr;

  for (unsigned y = 0; y < img.height(); y++) {
    head->north = prev;
    HSLAPixel* pixel = img.getPixel(0, y);
    head->colour.h = pixel->h;
    head->colour.s = pixel->s;
    head->colour.l = pixel->l;
    head->colour.a = pixel->a;
    ListForY(img, head, y);
    prev = head;
    if (y < (img.height() - 1)) {
      head = new ImgNode();
      prev->south = head;
    }
  }
  
  while (head->east != nullptr) {
    head = head->east;
  }
  southeast = head;
  head = prev= nullptr;

  ImgNode* top = northwest;
  ImgNode* bottom = northwest->south;

  for (unsigned y = 0; y < (img.height() - 1); y++) {
    ImgNode* topFirstColumn = top;
    while (top != nullptr){
      top->south = bottom;
      bottom->north = top;  
      top = top->east;
      bottom = bottom->east;
    }
    top = topFirstColumn->south;
    bottom = top->south;
  }
}

/*
* Copy constructor.
* Creates this this to become a separate copy of the data in otherlist
*/
ImgList::ImgList(const ImgList& otherlist) {
  // build the linked node structure using otherlist as a template
  Copy(otherlist);
}

/*
* Assignment operator. Enables statements such as list1 = list2;
*   where list1 and list2 are both variables of ImgList type.
* POST: the contents of this list will be a physically separate copy of rhs
*/
ImgList& ImgList::operator=(const ImgList& rhs) {
  // Re-build any existing structure using rhs as a template
  
  if (this != &rhs) { // if this list and rhs are different lists in memory
    // release all existing heap memory of this list
    Clear();    
    
    // and then rebuild this list using rhs as a template
    Copy(rhs);
  }
  
  return *this;
}

/*
* Destructor.
* Releases any heap memory associated with this list.
*/
ImgList::~ImgList() {
  // Ensure that any existing heap memory is deallocated
  Clear();
}

/************
* ACCESSORS *
************/

/*
* Returns the horizontal dimension of this list (counted in nodes)
* Note that every row will contain the same number of nodes, whether or not
*   the list has been carved.
* We expect your solution to take linear time in the number of nodes in the
*   x dimension.
*/
unsigned int ImgList::GetDimensionX() const {
  unsigned int dimensionX = 0;
  for (ImgNode* curr = northwest; curr != nullptr; curr = curr->east){
    dimensionX += 1;
  }
  return dimensionX;
}

/*
* Returns the vertical dimension of the list (counted in nodes)
* It is useful to know/assume that the grid will never have nodes removed
*   from the first or last columns. The returned value will thus correspond
*   to the height of the PNG image from which this list was constructed.
* We expect your solution to take linear time in the number of nodes in the
*   y dimension.
*/
unsigned int ImgList::GetDimensionY() const {
  unsigned int maxDimensionY = 0;
  for (ImgNode* curr = northwest; curr != nullptr; curr = curr->south) {
    maxDimensionY += 1;
  }  
  return maxDimensionY;
}

/*
* Returns the horizontal dimension of the list (counted in original pixels, pre-carving)
* The returned value will thus correspond to the width of the PNG image from
*   which this list was constructed.
* We expect your solution to take linear time in the number of nodes in the
*   x dimension.
*/
unsigned int ImgList::GetDimensionFullX() const {
  unsigned int dimensionX = GetDimensionX();
  unsigned int skips = 0;
  
  for (ImgNode* curr = northwest; curr != nullptr; curr = curr->east) {
    if (curr->skipleft != 0) {
      skips += curr->skipleft;
    }
    if (curr->skipright != 0) {
      skips +=  curr->skipright;
    }
  }


  return dimensionX + skips / 2;
}

/*
* Returns a pointer to the node which best satisfies the specified selection criteria.
* The first and last nodes in the row cannot be returned.
* PRE: rowstart points to a row with at least 3 physical nodes
* PRE: selectionmode is an integer in the range [0,1]
* PARAM: rowstart - pointer to the first node in a row
* PARAM: selectionmode - criterion used for choosing the node to return
*          0: minimum luminance across row, not including extreme left or right nodes
*          1: node with minimum total of "hue difference" with its left neighbour and with its right neighbour.
*        In the (likely) case of multiple candidates that best match the criterion,
*        the left-most node satisfying the criterion (excluding the row's starting node)
*        will be returned.
* A note about "hue difference": For PA1, consider the hue value to be a double in the range [0, 360).
* That is, a hue value of exactly 360 should be converted to 0.
* The hue difference is the absolute difference between two hues,
* but be careful about differences spanning the 360 value.
* e.g. Two pixels with hues 90 and 110 differ by 20, but
*      two pixels with hues 5 and 355 differ by 10.
*/
ImgNode* ImgList::SelectNode(ImgNode* rowstart, int selectionmode) {
  ImgNode* result = rowstart->east;

  if (selectionmode == 0) {
    double minL = result->colour.l;
    for (ImgNode* curr= rowstart->east; curr != nullptr; curr = curr->east) {
      if (curr->east == nullptr) {
        break;
      } else if (curr->colour.l < minL) {
        minL = curr->colour.l;
        result = curr;
      }
    }
  } else {
    double minHDiff = HueDiff(rowstart->colour.h, rowstart->east->colour.h) + HueDiff(rowstart->east->colour.h, rowstart->east->east->colour.h);    
    for (ImgNode* curr = rowstart->east; curr != nullptr; curr = curr->east) {
      if (curr->east == nullptr) {
        break;
      } else if ((HueDiff(curr->colour.h, curr->west->colour.h) + HueDiff(curr->colour.h, curr->east->colour.h)) < minHDiff) {
        minHDiff = HueDiff(curr->colour.h, curr->west->colour.h) + HueDiff(curr->colour.h, curr->east->colour.h);
        result = curr;
      }
    }
  } 
  return result;
}

/*
* Renders this list's pixel data to a PNG, with or without filling gaps caused by carving.
* PRE: fillmode is an integer in the range of [0,2]
* PARAM: fillgaps - whether or not to fill gaps caused by carving
*          false: render one pixel per node, ignores fillmode
*          true: render the full width of the original image,
*                filling in missing nodes using fillmode
* PARAM: fillmode - specifies how to fill gaps
*          0: solid, uses the same colour as the node at the left of the gap
*          1: solid, using the averaged values (all channels) of the nodes at the left and right of the gap
*             Note that "average" for hue will use the closer of the angular distances,
*             e.g. average of 10 and 350 will be 0, instead of 180.
*             Average of diametric hue values will use the smaller of the two averages
*             e.g. average of 30 and 210 will be 120, and not 300
*                  average of 170 and 350 will be 80, and not 260
*          2: *** OPTIONAL - FOR BONUS ***
*             linear gradient between the colour (all channels) of the nodes at the left and right of the gap
*             e.g. a gap of width 1 will be coloured with 1/2 of the difference between the left and right nodes
*             a gap of width 2 will be coloured with 1/3 and 2/3 of the difference
*             a gap of width 3 will be coloured with 1/4, 2/4, 3/4 of the difference, etc.
*             Like fillmode 1, use the smaller difference interval for hue,
*             and the smaller-valued average for diametric hues
*/
PNG ImgList::Render(bool fillgaps, int fillmode) const {
  PNG outpng; //this will be returned later. Might be a good idea to resize it at some point.
  
  if (!fillgaps) {

   
    outpng.resize(GetDimensionX(),GetDimensionY());
    ImgNode* moveY = northwest;
    for (unsigned y = 0; y < outpng.height(); y++) {
      ImgNode* moveX = moveY;
      for (unsigned x = 0; x < outpng.width(); x++) {
        HSLAPixel* pixel = outpng.getPixel(x, y);
        pixel->h = moveX->colour.h;
        pixel->s = moveX->colour.s;
        pixel->l = moveX->colour.l;
        pixel->a = moveX->colour.a;
        moveX = moveX->east;
      }
      moveY = moveY->south;
    }
  } else {
      outpng.resize(GetDimensionFullX(), GetDimensionY());
      ImgNode* moveY = northwest;
      for (unsigned y = 0; y < outpng.height(); y++) {
        ImgNode* moveX = moveY;
        for (unsigned x = 0; x < outpng.width(); x++) {
          HSLAPixel* pixel = outpng.getPixel(x, y);
          pixel->h = moveX->colour.h;
          pixel->s = moveX->colour.s;
          pixel->l = moveX->colour.l;
          pixel->a = moveX->colour.a;

          if (moveX->skipright != 0) {
            for (unsigned int skips = 1; skips <= moveX->skipright; skips++) {
              x += 1;
              HSLAPixel* pixel = outpng.getPixel(x, y);
              if (fillmode == 2) {
                FillGapsMode2(moveX, pixel, skips);
              } else {
                FillGaps(moveX, pixel, fillmode);
              }
            }
          }
          moveX = moveX->east;
        }
        moveY = moveY->south;
      }
    }
  return outpng;
}

/************
* MODIFIERS *
************/

/*
* Removes exactly one node from each row in this list, according to specified criteria.
* The first and last nodes in any row cannot be carved.
* PRE: this list has at least 3 nodes in each row
* PRE: selectionmode is an integer in the range [0,1]
* PARAM: selectionmode - see the documentation for the SelectNode function.
* POST: this list has had one node removed from each row. Neighbours of the created
*       gaps are linked appropriately, and their skip values are updated to reflect
*       the size of the gap.
*/
void ImgList::Carve(int selectionmode) {
  ImgNode* moveAlongColumn = northwest;

  while (moveAlongColumn != nullptr) {
    ImgNode* selectedNode = SelectNode(moveAlongColumn, selectionmode);
    //update skip values
    selectedNode->west->skipright += 1 + selectedNode->skipright;
    selectedNode->east->skipleft = selectedNode->west->skipright;
    if (selectedNode->north != nullptr) {
      selectedNode->north->skipdown += 1 + selectedNode->skipdown;
    }
    if (selectedNode->south != nullptr) {
      selectedNode->south->skipup += 1 + selectedNode->skipup;
    }

    //adjust links
    selectedNode->west->east = selectedNode->east;
    selectedNode->east->west = selectedNode->west;
    if (selectedNode->north != nullptr && selectedNode->south != nullptr) {
      selectedNode->north->south = selectedNode->south;
      selectedNode->south->north = selectedNode->north;
    }  
    if (selectedNode->north != nullptr && selectedNode->south == nullptr) {
        selectedNode->north->south = nullptr;
    }
    if (selectedNode->north == nullptr && selectedNode->south != nullptr) {
      selectedNode->south->north = nullptr;
    } 
    moveAlongColumn = moveAlongColumn->south;
    delete selectedNode;
    selectedNode = nullptr;
  }
}

// note that a node on the boundary will never be selected for removal
/*
* Removes "rounds" number of nodes (up to a maximum of node width - 2) from each row,
* based on specific selection criteria.
* Note that this should remove one node from every row, repeated "rounds" times,
* and NOT remove "rounds" nodes from one row before processing the next row.
* PRE: selectionmode is an integer in the range [0,1]
* PARAM: rounds - number of nodes to remove from each row
*        If rounds exceeds node width - 2, then remove only node width - 2 nodes from each row.
*        i.e. Ensure that the final list has at least two nodes in each row.
* POST: this list has had "rounds" nodes removed from each row. Neighbours of the created
*       gaps are linked appropriately, and their skip values are updated to reflect
*       the size of the gap.
*/
void ImgList::Carve(unsigned int rounds, int selectionmode) {
  rounds = (rounds > GetDimensionX() - 2) ? GetDimensionX() - 2 : rounds;

  for (unsigned int i = 1; i <= rounds; i++) {
    Carve(selectionmode);
  }
}

/*
* Helper function deallocates all heap memory associated with this list,
* puts this list into an "empty" state. Don't forget to set your member attributes!
* POST: this list has no  headently allocated nor leaking heap memory,
*       member attributes have values consistent with an empty list.
*/
void ImgList::Clear() {
  ImgNode* moveY = northwest;

  while (moveY != nullptr) {
    ImgNode* moveX = moveY;
    moveY = moveY->south;
    while (moveX != nullptr) {
      ImgNode* temp = moveX->east;
      delete moveX;
      moveX = temp;
    }
  }
  northwest = southeast = nullptr;
}

/* ************************
*  * OPTIONAL - FOR BONUS *
** ************************
* Helper function copies the contents of otherlist and sets this list's attributes appropriately
* PRE: this list is empty
* PARAM: otherlist - list whose contents will be copied
* POST: this list has contents copied from by physically separate from otherlist
*/
void ImgList::Copy(const ImgList& otherlist) {
  northwest = new ImgNode();
  ImgNode* head = northwest;
  ImgNode* prev = nullptr;

    // copy the first column of otherlist
  for (ImgNode* otherListMoveY = otherlist.northwest; otherListMoveY != nullptr; otherListMoveY = otherListMoveY->south) {
    head->colour.h = otherListMoveY->colour.h;
    head->colour.l = otherListMoveY->colour.l;
    head->colour.s = otherListMoveY->colour.s;
    head->colour.a = otherListMoveY->colour.a;
    head->skipup = otherListMoveY->skipup;
    head->skipdown = otherListMoveY->skipdown;
    head->skipleft = otherListMoveY->skipleft;
    head->skipright = otherListMoveY->skipright;

    if (prev != nullptr) {
      prev->south = head;
      head->north = prev;
    }

    prev = head;
    if (otherListMoveY->south != nullptr)  {
        head = new ImgNode();
    }
  }
  head = prev = nullptr;
  
  // Starting from the first node in each row, copy the node from otherlist if no gaps; otherwise, create an empty node for each gap
  ImgNode* curr;
  head = northwest;

  for (ImgNode* otherListMoveY = otherlist.northwest; otherListMoveY != nullptr; otherListMoveY = otherListMoveY->south) {
    ImgNode* prev = head;
    for (ImgNode* otherListMoveX = otherListMoveY; otherListMoveX != nullptr; otherListMoveX = otherListMoveX->east) {

      if (otherListMoveX->east == nullptr) {
        continue;
      } else {
        if (otherListMoveX->skipright == 0) {
        curr = new ImgNode();
        curr->colour.h = otherListMoveX->east->colour.h;
        curr->colour.l = otherListMoveX->east->colour.l;
        curr->colour.s = otherListMoveX->east->colour.s;
        curr->colour.a = otherListMoveX->east->colour.a;
        curr->skipup = otherListMoveX->east->skipup;
        curr->skipdown = otherListMoveX->east->skipdown;
        curr->skipleft = otherListMoveX->east->skipleft;
        curr->skipright = otherListMoveX->east->skipright;

        prev->east = curr;
        curr->west = prev;
        prev = curr;
        } else {
        for (unsigned int i = 1; i <= otherListMoveX->skipright; i++) {
          curr = new ImgNode();
          prev->east = curr;
          curr->west = prev;
          prev = curr;
        }
        curr = new ImgNode();
        curr->colour.h = otherListMoveX->east->colour.h;
        curr->colour.l = otherListMoveX->east->colour.l;
        curr->colour.s = otherListMoveX->east->colour.s;
        curr->colour.a = otherListMoveX->east->colour.a;
        curr->skipup = otherListMoveX->east->skipup;
        curr->skipdown = otherListMoveX->east->skipdown;
        curr->skipleft = otherListMoveX->east->skipleft;
        curr->skipright = otherListMoveX->east->skipright; 
        prev->east = curr;
        curr->west = prev;
        prev = curr;
        }
      }
    }
    head = head->south;
  }
  delete head;
  // Link all nodes vertically
  southeast = curr;
  delete prev;
  curr = prev= nullptr;

  ImgNode* top = northwest;
  ImgNode* bottom = northwest->south;

  for (unsigned int y = 0; y < (GetDimensionY() - 1); y++) {
    ImgNode* topFirstColumn = top;
    while (top != nullptr) {
      top->south = bottom;
      bottom->north = top;  
      top = top->east;
      bottom = bottom->east;
    }
    top = topFirstColumn->south;
    bottom = top->south;
  }

  // Remove the nodes that do not exist in otherlist by checking skip values
  for (ImgNode* moveAlongY = northwest; moveAlongY != nullptr; moveAlongY = moveAlongY->south) {
    for (ImgNode* moveAlongX = moveAlongY; moveAlongX != nullptr; moveAlongX = moveAlongX->east) {
      if (moveAlongX->skipright != 0) {
        ImgNode* nodeToCarve = moveAlongX->east;
        for (unsigned int skips = 1; skips <= moveAlongX->skipright; skips++) {
          CarveForCopy(nodeToCarve);
          nodeToCarve = nodeToCarve->east;
        }
        if (nodeToCarve == nullptr) {delete nodeToCarve;}
      }
    }
  }
}

/*************************************************************************************************
* IF YOU DEFINED YOUR OWN PRIVATE FUNCTIONS IN imglist.h, YOU MAY ADD YOUR IMPLEMENTATIONS BELOW *
*************************************************************************************************/
void ImgList::ListForY(PNG& img, ImgNode*& head, unsigned y) {
  ImgNode* prev = head;

  for (unsigned x = 1; x < img.width(); x++) {
    HSLAPixel* pixel = img.getPixel(x, y);
    ImgNode*  head = new ImgNode();
    head->colour.h = pixel->h;
    head->colour.s = pixel->s;
    head->colour.l = pixel->l;
    head->colour.a = pixel->a;

    prev->east =  head;
    head->west = prev;
    prev =  head;
  }
}

/*
 * Helper function for the copy constructor
 * PARM: selectedNode - pointer to the node that should be removed
 * POST: links to neighbour nodes have been adjusted, skip values of neighbour nodes remain unchanged
 */
void ImgList::CarveForCopy(ImgNode*& selectedNode) {
    selectedNode->west->east = selectedNode->east;
    selectedNode->east->west = selectedNode->west;
    if (selectedNode->north != nullptr && selectedNode->south != nullptr) {
      selectedNode->north->south = selectedNode->south;
      selectedNode->south->north = selectedNode->north;
    }  
    if (selectedNode->north != nullptr && selectedNode->south == nullptr) {
        selectedNode->north->south = nullptr;
    }
    if (selectedNode->north == nullptr && selectedNode->south != nullptr) {
      selectedNode->south->north = nullptr;
    } 
    
    delete selectedNode;
}

void ImgList::FillGaps (ImgNode* leftOfGap, HSLAPixel*& pixel, int fillmode) const {
  ImgNode* rightOfGap = leftOfGap->east;

  if (fillmode == 0) {
    pixel->h = leftOfGap->colour.h;
    pixel->l = leftOfGap->colour.l;
    pixel->s = leftOfGap->colour.s;
    pixel->a = leftOfGap->colour.a;
  } else {
    if (HueDiff(leftOfGap->colour.h, rightOfGap->colour.h) == 180.0) {
      pixel->h = fmin(leftOfGap->colour.h, rightOfGap->colour.h) + 90.0;
      pixel->h = pixel->h >= 180.0 ? pixel->h - 180 : pixel->h;
    } else if (fabs(leftOfGap->colour.h - rightOfGap->colour.h) > 180.0) {
      double difference = HueDiff(leftOfGap->colour.h, rightOfGap->colour.h) > 180 ? 
                          360 - HueDiff(leftOfGap->colour.h, rightOfGap->colour.h) : HueDiff(leftOfGap->colour.h, rightOfGap->colour.h);
      pixel->h = fmax(leftOfGap->colour.h, rightOfGap->colour.h) + difference / 2.0;
      
      if (pixel->h >= 360.0){
        pixel->h = difference / 2.0 - 360.0 + fmax(leftOfGap->colour.h, rightOfGap->colour.h);
      } 
    } else {
        pixel->h = (leftOfGap->colour.h + rightOfGap->colour.h) / 2;
    }
    pixel->s = (leftOfGap->colour.s + rightOfGap->colour.s) / 2;
    pixel->l = (leftOfGap->colour.l + rightOfGap->colour.l) / 2;
    pixel->a = (leftOfGap->colour.a + rightOfGap->colour.a) / 2;
  } 
}

void ImgList::FillGapsMode2 (ImgNode* leftOfGap, HSLAPixel*& pixel, int skipNum) const {
  ImgNode* rightOfGap = leftOfGap->east;
  unsigned int gapSize = leftOfGap->skipright;

  //colour.l
  double ldiff = fabs(leftOfGap->colour.l - rightOfGap->colour.l);
  if (leftOfGap->colour.l <= rightOfGap->colour.l) {
    pixel->l = leftOfGap->colour.l + ldiff * skipNum / (gapSize + 1); 
  } else {
    pixel->l = leftOfGap->colour.l - ldiff * skipNum / (gapSize + 1); 
  }

  //color.a
  double adiff = fabs(leftOfGap->colour.a - rightOfGap->colour.a);
  if (leftOfGap->colour.a <= rightOfGap->colour.a) {
    pixel->a = leftOfGap->colour.a + adiff * skipNum / (gapSize + 1); 
  } else {
    pixel->a = leftOfGap->colour.a - adiff * skipNum / (gapSize + 1); 
  }

  //colour.s
  double sdiff = fabs(leftOfGap->colour.s - rightOfGap->colour.s);
  if (leftOfGap->colour.s <= rightOfGap->colour.s) {
    pixel->s = leftOfGap->colour.s + sdiff * skipNum / (gapSize + 1); 
  } else {
    pixel->s = leftOfGap->colour.s - sdiff * skipNum / (gapSize + 1); 
  }

  //colour.h
  double hdiff;
  if (HueDiff(leftOfGap->colour.h, rightOfGap->colour.h) == 180) {
    hdiff = 180;
    pixel->h = fmin(leftOfGap->colour.h + hdiff * skipNum / (gapSize + 1), leftOfGap->colour.h - hdiff * skipNum / (gapSize + 1));
  
  } else {
    hdiff = HueDiff(leftOfGap->colour.h, rightOfGap->colour.h) > 180 ? 
                  360 - HueDiff(leftOfGap->colour.h, rightOfGap->colour.h) : HueDiff(leftOfGap->colour.h, rightOfGap->colour.h);
    if (fabs(leftOfGap->colour.h - rightOfGap->colour.h) < 180.0) {
      if (leftOfGap->colour.h <= rightOfGap->colour.h) {
        pixel->h = leftOfGap->colour.h + hdiff * skipNum / (gapSize + 1); 
      } else {
        pixel->h = leftOfGap->colour.h - hdiff * skipNum / (gapSize + 1); 
      }
    } else {
      if (leftOfGap->colour.h <= rightOfGap->colour.h) {
        pixel->h = leftOfGap->colour.h - hdiff * skipNum / (gapSize + 1) < 0 ? 
                    360 + leftOfGap->colour.h - hdiff * skipNum / (gapSize + 1) : leftOfGap->colour.h - hdiff * skipNum / (gapSize + 1);
      } else {
        pixel->h = leftOfGap->colour.h + hdiff * skipNum / (gapSize + 1) > 360 ?
                    leftOfGap->colour.h + hdiff * skipNum / (gapSize + 1) - 360 : leftOfGap->colour.h + hdiff * skipNum / (gapSize + 1);
      }
    }
  }
}