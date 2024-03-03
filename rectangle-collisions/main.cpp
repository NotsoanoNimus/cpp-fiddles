/*
 * Small program to play around with the standard library and
 *   detecting rectangle collisions within single-digit millisecond
 *   time intervals, using a 'sweep' style algorithm.
 *
 * Uses "randomly" generated rectangles with the built-in 'rand'.
 *
 * This is needed for some intense work I'm doing right now at
 *   my primary job, working on display refresh rates of up to
 *   100 Hz. Need an algorithm to detect when two or more rectangles
 *   are colliding and fetch the extents of the composite rectangle
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
    void print() const {
        printf("(%d, %d) -> (%d, %d) [%d x %d]",
               x, y, x + width, y + height, width, height);
    }
};

int randomInt(int, int);
std::vector<Rect *> getRectangleExtents(int, std::vector<double> &, const std::vector<Rect *> &);
void manualCollisionTests();


int
main()
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
        auto extents = getRectangleExtents(
                STEP,
                allDurations,
                std::vector<Rect *>(allRectangles.begin() + j, allRectangles.begin() + j + STEP)
        );

        // No need to do anything with these extents for this program.
//        for (auto &extent : extents) free(extent);
    }

//    for (const auto &item: allRectangles)
//        free(item);

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


int
randomInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}


static inline bool
getCollisionExtentsIfIntersection(Rect &currentExtent, Rect *rightRect)
{
    //printf("\tCOMPARE: "); currentExtent.print(); printf(" // "); rightRect->print(); printf("\n");

    // Check to see if the origin of 'rightRect' is within the x value range of 'leftRect'.
    if (rightRect->x >= currentExtent.x && rightRect->x < (currentExtent.x + currentExtent.width)) {
        // If so, need to check 'rightRect' left-side points for any intersection
        //   with the y value range of 'leftRect'. If there's a match, then there's a collision.
        //
        // This uses a trick for one-dimensional number lines to find if two lines overlap at all...
        //   max(start1, start2) < min(end1, end2)
        if (std::max(currentExtent.y, rightRect->y) < std::min(currentExtent.y + currentExtent.height, rightRect->y + rightRect->height)) {
            //printf("\t\tCollision!\n");
            currentExtent = {
                currentExtent.x,
                std::min(currentExtent.y, rightRect->y),
                std::max(currentExtent.x + currentExtent.width, rightRect->x + rightRect->width) - currentExtent.x,
                std::max(currentExtent.y + currentExtent.height, rightRect->y + rightRect->height) - std::min(currentExtent.y, rightRect->y),
            };

            return true;
        }
    }

    return false;
}


std::vector<Rect *>
getRectangleExtents(
        int previousExtentsCount,
        std::vector<double> &durations,
        const std::vector<Rect *> &inputList)
{
    unsigned long long inputListSize;
    std::vector<Rect *> extents;
    std::vector<Rect *> sortedByOriginX {inputList};

    // Set up variables and print some preliminary details.
    inputListSize = inputList.size();
    printf("\tAnalyzing %llu rectangles...\n", inputListSize);
    for (auto &rect : inputList) {
        printf("\t\t"); rect->print(); printf("\n");
    }

    // Start the clock.
    auto startTime = std::chrono::high_resolution_clock::now();

    // Short-circuit if the list isn't big enough to iterate.
    if (inputListSize < 1) {
        throw std::exception();
    } else if (inputListSize == 1) {
        return inputList;
    }

    // Re-sort the input list by origin-X position.
    std::sort(sortedByOriginX.begin(), sortedByOriginX.end(),
              [](Rect *a, Rect *b) -> bool { return b->x > a->x; });

    // Shorthand some iterator positions.
    auto it = sortedByOriginX.begin();
    auto end = sortedByOriginX.end();

    // Loop over the set but with a manual control of the iterator.
    //   Each time the getCollision... function returns 'true', it keeps iterating for the next collision
    //   before adding the entire resulting rectangle to the
    do {
        Rect *left = *it;
        while (++it != end && getCollisionExtentsIfIntersection(*left, *it));
        extents.push_back(left);
    } while (it != end);

    // Take some timing measurements.
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(endTime - startTime);
    durations.push_back(duration.count());

    // If the amount of rectangle extents has changed, need to iterate the list again for further
    //   merges of the extents list. Otherwise, there will be missing overlays/collisions.
    int amount = (int)extents.size();
    return previousExtentsCount != amount
        ? getRectangleExtents(amount, durations, extents)
        : extents;
}


void
manualCollisionTests()
{
    int testNumber = 0;
    auto runTestsFunc = [&testNumber](const char *message, const std::vector<Rect *> &inputs) -> std::vector<Rect *> {
        ++testNumber;
        printf("\nTEST %3d: %s\n", testNumber, message);
        
        std::vector<double> timer;
        auto extents = getRectangleExtents((int)inputs.size(), timer, inputs);

        printf("\t==> Extents: %d\n", (int)extents.size());
        for (auto &extent : extents) {
            printf("\t\t"); extent->print(); printf("\n");
        }

        printf("Tests completed in %f seconds.\n",
               std::accumulate(timer.begin(), timer.end(), 0.0));

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

    runTestsFunc("Triple collision, direct.", {
            new Rect{10, 10, 20, 10},
            new Rect{8, 5, 40, 40},
            new Rect{17, 0, 100, 100}
    });

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

    // A set of squares and rectangles, none of which collide, except
    //   there is an overlay shape which collides with all and creates
    //   a single extent.
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

    // Four simple sprites. None are touching
    runTestsFunc("Four simple sprites (four extents; no collisions).", {
            new Rect{0, 0, 5, 20},
            new Rect{150, 150, 10, 20},
            new Rect{942, 238, 10, 20},
            new Rect{10, 1550, 10, 20},
    });

    // Four simple sprites. Two of them are colliding, third with its extent, and fourth is alone.
    runTestsFunc("Four simple sprites (two extents).", {
            new Rect{138, 165, 5, 20},
            new Rect{150, 150, 10, 20},
            new Rect{142, 138, 10, 20},
            new Rect{10, 10, 10, 20},
    });
}
