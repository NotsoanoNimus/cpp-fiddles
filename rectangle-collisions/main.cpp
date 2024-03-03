/*
 * Small program to play around with the standard library and
 *   detecting rectangle collisions within single-digit millisecond
 *   time intervals, using a 'sweep' style algorithm.
 *
 * Uses "randomly" generated rectangles with the built-in 'rand'.
 *
 * This is needed for some intense work I'm doing right now at
 *   my primary job, working on display refresh rates of up to
 *   100 Hz. Need an algorithm to detect when two rectangles are
 *   colliding and fetch the extents of the composite rectangle
 *   bounds to update on the display.
 *
 *
 * Author: Zachary Puhl
 * Date  : Saturday, March 02, 2024
 */

#include <iostream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <numeric>

// Set RANDOM_RECT_BATCHES to 0 if not doing timing tests...
#define STEP 4
#define RANDOM_RECT_BATCHES  (0 * STEP)

#define MIN_X      0
#define MAX_X      300
#define MIN_Y      0
#define MAX_Y      300
#define MIN_WIDTH  1
#define MAX_WIDTH  200
#define MIN_HEIGHT 1
#define MAX_HEIGHT 200


struct Rect
{
    int x;
    int y;
    int width;
    int height;
    std::vector<Rect *> detectedCollisions {};
    void print() const {
        printf("(%d, %d) -> (%d, %d) [%d x %d]",
               x, y, x + width, y + height, width, height);
    }
};

int randomInt(int, int);
std::vector<Rect *> getIntersectionExtents(std::vector<double>&, const std::vector<Rect *>&);
void manualCollisionTests();


int main()
{
    srand(time(nullptr));

    std::vector<double> allDurations;
    std::vector<Rect *> allRectangles;

    // Generate some random rectangles.
    for (int i = 0; i < RANDOM_RECT_BATCHES; ++i)
        allRectangles.push_back(new Rect(
            randomInt(MIN_X, MAX_X),
            randomInt(MIN_Y, MAX_Y),
            randomInt(MIN_WIDTH, MAX_WIDTH),
            randomInt(MIN_HEIGHT, MAX_HEIGHT)
        ));

    printf("Loaded %d rectangles; using %d steps...\n", (int)allRectangles.size(), STEP);

    // Sample [STEP] rectangles from the set at a time.
    for (int j = 0; j < allRectangles.size() - (allRectangles.size() % STEP); j += STEP) {
        auto extents = getIntersectionExtents(
                allDurations,
                std::vector<Rect *>(allRectangles.begin() + j, allRectangles.begin() + j + STEP)
        );

        // No need to do anything with these extents for this program.
        for (auto &extent : extents) free(extent);
    }

    for (const auto &item: allRectangles)
        free(item);

    printf("\n\nCompleted in %f seconds.\n\n=== ALL DONE. ===\n",
           std::accumulate(allDurations.begin(), allDurations.end(), 0.0));

    // Run some manual tail tests.
    manualCollisionTests();
    printf("\n\nCompleted tests.\n>>>>> IT IS UP TO YOU TO MANUALLY VERIFY THESE. <<<<<\n");

    return 0;
}


// ========================================================================================
// ========================================================================================
// ========================================================================================
// ========================================================================================


int randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}


std::tuple<bool, Rect *> getCollisionExtentsIfIntersection(Rect *leftRect, Rect *rightRect)
{
    printf("\tCOMPARE: "); leftRect->print(); printf(" // "); rightRect->print(); printf("\n");

    // Check to see if the origin of 'rightRect' is within the x value range of 'leftRect'.
    if (rightRect->x >= leftRect->x && rightRect->x < (leftRect->x + leftRect->width)) {
        // If so, need to check 'rightRect' left-side points for any intersection
        //   with the y value range of 'leftRect'. If there's a match, then there's a collision.
        //
        // This uses a trick for one-dimensional number lines to find if two lines overlap at all...
        //   max(start1, start2) < min(end1, end2)
        if (std::max(leftRect->y, rightRect->y) < std::min(leftRect->y + leftRect->height, rightRect->y + rightRect->height)) {
            printf("\t\tCollision!\n");
            std::tuple<bool, Rect *> newExtent{
                    true,
                    new Rect{
                            leftRect->x,
                            std::min(leftRect->y, rightRect->y),
                            std::max(leftRect->width, rightRect->width),
                            std::max(leftRect->height, rightRect->height),
                    }
            };

            return newExtent;
        }
    }

    return {false, leftRect};
}


std::vector<Rect *> getRectangleExtents(
        std::vector<double> &durations,
        const std::vector<Rect *> &inputList)
{
    unsigned long long inputListSize;
    std::vector<Rect *> extents;
    std::vector<Rect *> sortedByOriginX {inputList};

    // Make sure the size of the list is worth checking for extents.
    inputListSize = inputList.size();
    printf("\tAnalyzing %llu rectangles...\n", inputListSize);
    for (auto &rect : inputList) {
        printf("\t\t"); rect->print(); printf("\n");
    }

    if (inputListSize < 1) {
        throw std::exception();
    } else if (inputListSize == 1) {
        return inputList;
    }

    // Re-sort the input list by origin-X position.
    std::sort(sortedByOriginX.begin(), sortedByOriginX.end(), [](Rect *a, Rect *b) -> bool { return b->x > a->x; });

    // Get a list of rect <-> rect collisions. This will be reduced into another vector of Rect* extents.
    //   The function is recursive UNLESS there are no more collisions; as in, the reduced set will be fed back
    //   into this function.
//    std::vector<std::tuple<Rect *, Rect *>> collisionsMap;
//    std::vector<Rect *> collisionVictims;

    auto it = sortedByOriginX.begin();
    auto finalElement = std::prev(sortedByOriginX.end());

    do {
        Rect *left = *it;
        auto extent = getCollisionExtentsIfIntersection(left, *++it);

        while (std::get<0>(extent) && it != finalElement) {
            extent = getCollisionExtentsIfIntersection(std::get<1>(extent), *++it);
        }

        extents.push_back(std::get<1>(extent));
    } while (it != finalElement);

    return extents;

//    for (int i = 0; i < sortedByOriginX.size(); ++i) {
//        auto rect = sortedByOriginX[i];
//        bool isVictim = false;
//
//        // If this rectangle was already involved in a collision, skip it.
//        isVictim = collisionVictims.end() != std::find(collisionVictims.begin(), collisionVictims.end(), rect);
//        if (isVictim) continue;
//
//        std::vector<Rect *> slice(sortedByOriginX.begin() + i + 1, sortedByOriginX.end());
//        for (auto &comparedTo : slice) {
//            // Check to see if the origin of 'comparedTo' is within the x value range of 'rect'.
//            if (comparedTo->x >= rect->x && comparedTo->x < (rect->x + rect->width)) {
//                // If so, need to check 'comparedTo' left-side points for any intersection
//                //   with the y value range of 'rect'. If there's a match, then there's a collision.
//                //
//                // This uses a trick for one-dimensional number lines to find if two lines overlap at all...
//                //   max(start1, start2) < min(end1, end2)
//                if (std::max(rect->y, comparedTo->y) < std::min(rect->y + rect->height, comparedTo->y + comparedTo->height)) {
//                    collisionsMap.emplace_back(rect, comparedTo);
//
//                    collisionVictims.push_back(rect);
//                    collisionVictims.push_back(comparedTo);
//
//                    isVictim = true;
//                }
//            }
//        }
//
//        if (!isVictim) {
//            // Push the rectangle into the vector. Since it has no collisions, it is its own extent.
//            extents.push_back(rect);
//        }
//    }

//    // Reduce the set of collisions to a set of extents.
//    printf("\t==> Collisions: %d\n", (int)collisionsMap.size());
//    for (auto &collision : collisionsMap) {
//        // Since this was scanned left-to-right on the grid, we know the first tuple element is to the left.
//        Rect *leftRect = std::get<0>(collision);
//        Rect *rightRect = std::get<1>(collision);
//
//        printf("\t\t||");
//        leftRect->print();
//        printf("||  with  ||");
//        rightRect->print();
//        printf("||\n");
//
//        auto newExtent = new Rect {
//                leftRect->x,
//                std::min(leftRect->y, rightRect->y),
//                std::max(leftRect->x + leftRect->width, rightRect->x + rightRect->width),
//                std::max(leftRect->y + leftRect->height, rightRect->y + rightRect->height),
//        };
//        printf("\t\t\t"); newExtent->print(); printf("\n");
//        extents.push_back(newExtent);
//    }
//
//    // If there were any collision elements, the set of extents should be recalculated
//    //   recursively to merge new extents that overlap.
//    if (!collisionsMap.empty()) {
//        return getRectangleExtents(durations, extents);
//    }
//
//    return extents;
}


std::vector<Rect *> getRectangleExtents_BigO_n2(
        std::vector<double> &durations,
        const std::vector<Rect *> &inputList)
{
    // Time complexity of this isn't looking good...
    for (auto &rect : inputList) {
        for (auto &comparedToRect : inputList) {
            if (comparedToRect == rect) continue;

            bool alreadyCollided = comparedToRect->detectedCollisions.end() != std::find(comparedToRect->detectedCollisions.begin(),
                                                                                         comparedToRect->detectedCollisions.end(),
                                                                                         rect);
            if (alreadyCollided) continue;

            // Check to see if the origin of 'comparedToRect' is within the x value range of 'rect'.
            if (comparedToRect->x >= rect->x && comparedToRect->x < (rect->x + rect->width)) {
                // If so, need to check 'comparedToRect' left-side points for any intersection
                //   with the y value range of 'rect'. If there's a match, then there's a collision.
                //
                // This uses a trick for one-dimensional number lines to find if two lines overlap at all...
                //   max(start1, start2) < min(end1, end2)
                if (std::max(rect->y, comparedToRect->y) < std::min(rect->y + rect->height, comparedToRect->y + comparedToRect->height))
                    rect->detectedCollisions.push_back(comparedToRect);
            }
        }
    }

//    auto extentValueFunc = [matched](auto subLambda, auto minMaxLambda) -> int {
//        std::vector<int> values(matched.size());
//        std::transform(matched.begin(), matched.end(), values.begin(), subLambda);
//        return (int)minMaxLambda(values);
//    };
//
//    // Macro from HELL. I love it >:)
//    #define GET_EXTENT(name, lambda, method) \
//            auto (name) = extentValueFunc([](Rect *r) -> int { return (lambda); }, \
//                [](std::vector<int> v) -> int { return *(method)(v.begin(), v.end()); });
//
//    GET_EXTENT(minExtentX, r->x, std::min_element)
//    GET_EXTENT(minExtentY, r->y, std::min_element)
//    GET_EXTENT(maxExtentX, r->x + r->width, std::max_element)
//    GET_EXTENT(maxExtentY, r->y + r->height, std::max_element)
//
//    #undef GET_EXTENT
}


std::vector<Rect *> getIntersectionExtents(
        std::vector<double> &durations,
        const std::vector<Rect *> &inputList)
{
    // The returned list of extents is a set of rectangles which should be updated
    //   based on the inputList.
    std::vector<Rect *> extents;
    std::vector<Rect *> collidingRectangles;

    printf("\nSampling %d rectangles to find extent rectangles...\n", (int)inputList.size());
    for (auto &x : inputList) {
        printf("\t"); x->print(); printf("\n");
    }

    printf("\n");
    auto startTimeSlice = std::chrono::high_resolution_clock::now();

    for (auto &r : inputList) {
        std::vector<Rect *> matched;

        // Get the rectangles where the x-points and y-points
        for (auto &otherRect : inputList) {
            if (otherRect == r) continue;

            // Check to see if the origin of 'otherRect' is within the x value range of 'r'.
            if (otherRect->x >= r->x && otherRect->x < (r->x + r->width)) {
                // If so, need to check 'otherRects' left-side points for any intersection
                //   with the y value range of 'r'. If there's a match, then there's a collision.
                //
                // This uses a trick for one-dimensional number lines to find if two lines overlap at all...
                //   max(start1, start2) < min(end1, end2)
                if (std::max(r->y, otherRect->y) < std::min(r->y + r->height, otherRect->y + otherRect->height))
                    matched.push_back(otherRect);
            }
        }

        // If the matches set is empty, then there are no collisions.
        if (matched.empty()) {
            continue;
        }

        if (collidingRectangles.end() != std::find(collidingRectangles.begin(), collidingRectangles.end(), r))
            collidingRectangles.push_back(r);

        // Print collision details.
        for (auto &match : matched) {
            printf("\tCOLLISION: ||");
            r->print();
            printf("||  with  ||");
            match->print();
            printf("||\n");
        }

        // Calculate the extents of the resulting super-rectangle.
        // At this point, need to push >this< rectangle into the vector as well to get all possible extents.
        matched.push_back(r);

        auto extentValueFunc = [matched](auto subLambda, auto minMaxLambda) -> int {
            std::vector<int> values(matched.size());
            std::transform(matched.begin(), matched.end(), values.begin(), subLambda);
            return (int)minMaxLambda(values);
        };

        // Macro from HELL. I love it >:)
        #define GET_EXTENT(name, lambda, method) \
            auto (name) = extentValueFunc([](Rect *r) -> int { return (lambda); }, \
                [](std::vector<int> v) -> int { return *(method)(v.begin(), v.end()); });

        GET_EXTENT(minExtentX, r->x, std::min_element)
        GET_EXTENT(minExtentY, r->y, std::min_element)
        GET_EXTENT(maxExtentX, r->x + r->width, std::max_element)
        GET_EXTENT(maxExtentY, r->y + r->height, std::max_element)

        #undef GET_EXTENT

        extents.push_back(new Rect{minExtentX, minExtentY, (maxExtentX - minExtentX), (maxExtentY - minExtentY)});

        printf("\t\t===============\n");
        printf("\t\tEXTENTS: (%d, %d) to (%d, %d)\n", minExtentX, minExtentY, maxExtentX, maxExtentY);
        printf("\t\t===============\n\n");
    }

    auto endTimeSlice = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> durationSlice = endTimeSlice - startTimeSlice;

    printf("Completed in %f seconds.\n", durationSlice.count());
    durations.push_back(durationSlice.count());
    printf("=========================================\n");

    return extents;
}


void manualCollisionTests()
{
    int testNumber = 0;
    auto runTestsFunc = [&testNumber](const char *message, const std::vector<Rect *> &inputs) -> std::vector<Rect *> {
        ++testNumber;
        printf("\nTEST %3d: %s\n", testNumber, message);

        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<double> dummy;
        auto extents = getRectangleExtents(dummy, inputs);

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> durationSlice = endTime - startTime;
        
        for (auto &rect : inputs) free(rect);

        printf("\t==> Extents: %d\n", (int)extents.size());
        for (auto &extent : extents) {
            printf("\t\t"); extent->print(); printf("\n");
        }

        printf("Tests completed in %f seconds.\n", durationSlice.count());

        return extents;
    };

    runTestsFunc("Bottom-right quadrant.", {new Rect{0, 0, 100, 100}, new Rect{50, 50, 100, 100}});
    runTestsFunc("Top-right quadrant.", {new Rect{200, 200, 100, 100}, new Rect{250, 250, 100, 100}});
    runTestsFunc("Bar through center vertically.", {new Rect{50, 50, 200, 200}, new Rect{125, 25, 50, 250}});
    runTestsFunc("Bar through center horizontally.", {new Rect{350, 350, 200, 200}, new Rect{325, 425, 250, 50}});
    runTestsFunc("Bar partial insertion on left.", {new Rect{200, 0, 200, 200}, new Rect{25, 75, 200, 50}});
    runTestsFunc("Bar partial insertion on right.", {new Rect{0, 300, 200, 200}, new Rect{175, 375, 200, 50}});
    runTestsFunc("Bar partial insertion on top.", {new Rect{50, 50, 200, 200}, new Rect{125, 25, 50, 250}});
    runTestsFunc("Bar partial insertion on bottom.", {new Rect{350, 350, 200, 200}, new Rect{325, 425, 250, 50}});
    runTestsFunc("Fully subsumed horizontal bar.", {new Rect{0, 0, 200, 200}, new Rect{25, 75, 150, 50}});
    runTestsFunc("Fully subsumed vertical bar.", {new Rect{300, 300, 200, 200}, new Rect{375, 325, 50, 150}});
    runTestsFunc("Fully matched overlay.", {new Rect{0, 0, 200, 200}, new Rect{0, 0, 200, 200}});
    runTestsFunc("(no collision) Completely unrelated squares.", {new Rect{300, 300, 200, 200}, new Rect{600, 600, 200, 200}});
    runTestsFunc("(no collision) Bordering at bottom.", {new Rect{0, 0, 100, 100}, new Rect{0, 100, 100, 100}});
    runTestsFunc("(no collision) Bordering at right.", {new Rect{0, 0, 100, 100}, new Rect{100, 0, 100, 100}});
    runTestsFunc("(no collision) Corner points match bottom-right.", {new Rect{0, 0, 100, 100}, new Rect{100, 100, 100, 100}});
    runTestsFunc("(no collision) Corner points match top-right.", {new Rect{0, 100, 100, 100}, new Rect{100, 0, 100, 100}});
    runTestsFunc("1-pixel overlap at bottom.", {new Rect{0, 0, 100, 100}, new Rect{0, 99, 100, 100}});
    runTestsFunc("1-pixel overlap at right.", {new Rect{0, 0, 100, 100}, new Rect{99, 0, 100, 100}});
    runTestsFunc("Slight overlap at bottom-right corner.", {new Rect{0, 0, 100, 100}, new Rect{99, 99, 100, 100}});
    runTestsFunc("Slight overlap at top-right corner.", {new Rect{0, 99, 100, 100}, new Rect{99, 0, 100, 100}});

//    runTestsFunc("Triple collision.", {
//            new Rect{},
//            new Rect{},
//            new Rect{}
//    });

    // What should happen here is the first two will collide, and though the third doesn't directly collide,
    //   it collides with the extent of the first two, so it should be included.
    runTestsFunc("Triple collision; third is an extent collision.", {
            new Rect{0, 0, 100, 100},
            new Rect{50, 50, 100, 100},
            new Rect{125, 0, 300, 25}
    });

    // A set of squares and rectangles, none of which collide.
    runTestsFunc("Extent islands.", {
            new Rect{0, 0, 10, 10},
            new Rect{15, 0, 10, 10},
            new Rect{30, 0, 10, 10},
            new Rect{45, 0, 10, 10},
            new Rect{60, 0, 10, 10},
            new Rect{75, 0, 10, 10},
            new Rect{90, 0, 10, 10},
            new Rect{105, 0, 10, 10},
            new Rect{120, 0, 10, 10},
            new Rect{135, 0, 10, 10},
            new Rect{150, 0, 10, 10},
            new Rect{165, 0, 10, 10},
    });

    // A set of squares and rectangles, none of which collide.
    runTestsFunc("Extent islands with an overlay.", {
            new Rect{0, 0, 500, 500},
            new Rect{0, 0, 10, 10},
            new Rect{15, 0, 10, 10},
            new Rect{30, 0, 10, 10},
            new Rect{45, 0, 10, 10},
            new Rect{60, 0, 10, 10},
            new Rect{75, 0, 10, 10},
            new Rect{90, 0, 10, 10},
            new Rect{105, 0, 10, 10},
            new Rect{120, 0, 10, 10},
            new Rect{135, 0, 10, 10},
            new Rect{150, 0, 10, 10},
            new Rect{165, 0, 10, 10},
    });
}
