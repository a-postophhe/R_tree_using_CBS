#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define M 4
#define m 2

// Struct to define the MBR or the actual data point at the leaf
typedef struct Rectangle
{
    float x_min, y_min, x_max, y_max;
} Rectangle;

// Define a struct to represent a node in the R-tree
typedef struct Node
{
    bool is_leaf;
    int num_children;
    Rectangle *rectangle;
    struct Node **children;
    struct Node *parent;
} Node;

Node *CBSsplitNode(Node *root, Node *node, Rectangle *rectangle);                                   // done
Node *chooseLeaf(Node *node, Rectangle *rectangle);                                              // done
void insert(Node *node, Rectangle *rectangle);                                                   // done
Node *createNode(Node *parent, bool isLeaf, float x_min, float x_max, float y_min, float y_max); // done
Rectangle *newOverlappingRectangle(Rectangle *originalRect, Rectangle *toBeInsertedRect);        // done
Rectangle *createRectangle(float x_min, float x_max, float y_min, float y_max);                  // done
void adjustTree(Node *root, Node *node1, Node *node2);                                           // done
void search_node(Node *root, Rectangle *rectangle, Node **array, int counter);
double calculateEnlargement(Rectangle *rectangle, Rectangle *other);
double calculateOverlap(Rectangle *rectangle, Rectangle *other);
void printTree(Node *root); // done. need to check if its working fine
int calculateArea(Rectangle *rect);
void adjustRoot(Node *root);
// Node * getParent(Node * node);   not required as the structure has been changed to keep track of the parent

// function to create a rectangle based on a point
Rectangle *createRectangle(float x_min, float x_max, float y_min, float y_max)
{
    Rectangle *rect = (Rectangle *)malloc(sizeof(Rectangle));
    rect -> x_min = x_min;
    rect -> y_min = y_min;
    rect -> x_max = x_max;
    rect -> y_max = y_max;
    return rect;
}

// Function to create a new R-tree node
Node *createNode(Node *parent, bool is_leaf, float x_min, float x_max, float y_min, float y_max)
{
    Node *node = (Node *)malloc(sizeof(Node));
    node -> is_leaf = is_leaf;
    node -> num_children = 0;
    node -> children = (Node **)malloc(M * sizeof(Node *));
    node -> rectangle = createRectangle(x_min, x_max, y_min, y_max);
    node -> parent = parent; // NULL;
    return node;
}

void insert(Node *root, Rectangle *rectToBeInserted)
{
    // I1 [Find position for new record]
    Node *endNodeWhereRectIsToBeInserted = chooseLeaf(root, rectToBeInserted);

    // I2 [Add record to leaf node if it has space]
    if (endNodeWhereRectIsToBeInserted -> num_children < M)
    {
        endNodeWhereRectIsToBeInserted -> children[endNodeWhereRectIsToBeInserted -> num_children++] = createNode(endNodeWhereRectIsToBeInserted, true, rectToBeInserted -> x_min, rectToBeInserted -> x_max, rectToBeInserted -> y_min, rectToBeInserted -> y_max);
        endNodeWhereRectIsToBeInserted -> rectangle = newOverlappingRectangle(endNodeWhereRectIsToBeInserted -> rectangle, rectToBeInserted);
        adjustTree(root, endNodeWhereRectIsToBeInserted, NULL);
    }

    // If the end-node does not have enough space to add one more leaf (data - point)
    else
    {
        // To transfer data points while re-allocating the size of the node to be M + 1
        Node **child = (Node **)malloc(sizeof(Node *) * 4);
        for (int i = 0; i < 4; i++)
        {
            child[i] = endNodeWhereRectIsToBeInserted -> children[i];
        }

        endNodeWhereRectIsToBeInserted -> children = (Node **)malloc(sizeof(Node *) * (M + 1));

        for (int i = 0; i < 4; i++)
        {
            endNodeWhereRectIsToBeInserted -> children[i] = child[i];
        }

        // Add the child first and then split as per the CBS Algorithm
        endNodeWhereRectIsToBeInserted -> children[4] = createNode(endNodeWhereRectIsToBeInserted, true, rectToBeInserted -> x_min, rectToBeInserted -> x_max, rectToBeInserted -> y_min, rectToBeInserted -> y_max);
        endNodeWhereRectIsToBeInserted -> rectangle = newOverlappingRectangle(endNodeWhereRectIsToBeInserted -> rectangle, rectToBeInserted);

        // Adjust Tree is being called in the CBSsplitNode function itself
        CBSsplitNode(root, endNodeWhereRectIsToBeInserted, rectToBeInserted); // Split the L Node
        adjustRoot(root); // Kepp adjusting the root MBR
    }
}

// Adjust the root of the MBR
void adjustRoot(Node *root)
{
    for (int i = 0; i < root -> num_children; i++)
    {
        root -> rectangle = newOverlappingRectangle(root -> rectangle, root -> children[i] -> rectangle);
    }
}

// For adjusting the MBRs of the parent nodes, insert N2, as well as propagate till the root and keep splitting wherever required
void adjustTree(Node *root, Node *L, Node *NN)
{
    Node *firstNode = L;
    Rectangle *rect = firstNode -> children[0] -> rectangle;
    Node *temp = createNode(NULL, false, rect -> x_min, rect -> x_max, rect -> y_min, rect -> y_max);
    if (firstNode == root)
    {
        return;
    }

    // Second node will be inserted if its non null as per the algorithm
    Node *secondNode = NULL;
    if (NN != NULL)
    {
        secondNode = NN;
    }

    Node *P1 = firstNode -> parent;
    
    // adjust covering rectangle in parent entry
    for (int i = 1; i < firstNode -> num_children; i++)
    {
        temp -> rectangle = newOverlappingRectangle(temp -> rectangle, firstNode -> children[i] -> rectangle);
    }
    firstNode -> rectangle = temp -> rectangle;
    P1 -> rectangle = newOverlappingRectangle(P1 -> rectangle, firstNode -> rectangle);

    while (firstNode != root && firstNode != NULL)
    {
        P1 = firstNode -> parent;

        // propagate node split upward
        if (secondNode != NULL) // Add N2 as well
        {
            Rectangle *rect = secondNode -> children[0] -> rectangle;
            Node *ENN = createNode(secondNode, false, rect -> x_min, rect -> x_max, rect -> y_min, rect -> y_max);
            
            for (int i = 1; i < secondNode -> num_children; i++)
            {
                ENN -> rectangle = newOverlappingRectangle(ENN -> rectangle, secondNode -> children[i] -> rectangle);
            }

            secondNode -> rectangle = ENN -> rectangle;

            if (P1 -> num_children < M)
            {
                P1 -> children[P1 -> num_children++] = secondNode;
                secondNode -> parent = P1;
                break;
            }

            // In the case where its parent also need to be splitted
            else
            {        
                // To transfer data points while re-allocating the size of the node to be M + 1
                Node **child = (Node **)malloc(sizeof(Node *) * 4);
                for (int i = 0; i < 4; i++)
                {
                    child[i] = P1 -> children[i];
                }

                P1 -> children = (Node **)malloc(sizeof(Node *) * (M + 1));

                for (int i = 0; i < 4; i++)
                {
                    P1 -> children[i] = child[i];
                }
                
                P1 -> children[4] = secondNode;
                secondNode -> parent = P1;
                P1 -> num_children++;
                P1 -> rectangle = newOverlappingRectangle(P1 -> rectangle, ENN -> rectangle);
                Node *last_parent = P1;

                Node *PP = CBSsplitNode(root, P1, ENN -> rectangle);

                if (P1 == root)
                {
                    // In case the root was the node being split
                    root = PP;
                    break;
                }
            }
        }
        firstNode = P1 -> parent;
        // Adjust tree iteratively
    } // end of while
    adjustRoot(root);
}

// Calculate area of rectangle
int calculateArea(Rectangle *rect)
{
    int area = (rect -> x_max - rect -> x_min) * (rect -> y_max - rect -> y_min);
    return area;
}

// Main function of the code where CBS is being implemented
Node *CBSsplitNode(Node *root, Node *nodeToBeSplitted, Rectangle *rect)
{
    // Corners variable added to first assgn to which corner each of the node gets added to
    int corners[4] = {0};
    // To get the split axis -> false means y-split is taking place
    bool splitX = true;
    // To capture which node is being assigned to which corner
    int indexes[4][5] = {{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1}};
    Rectangle *center = newOverlappingRectangle(rect, nodeToBeSplitted -> rectangle);

    // Check added if the root was the one being split
    int check = 0;
    if (nodeToBeSplitted == root)
        check = 1;

    int center_x = (center -> x_min + center -> x_max) / 2;
    int center_y = (center -> y_min + center -> y_max) / 2;

    // center of the rectangle to be inserted
    int rect2_x = (rect -> x_min + rect -> x_max) / 2;
    int rect2_y = (rect -> y_min + rect -> y_max) / 2;

    // Loop to assign nodes to the corners
    for (int i = 0; i < 5; i++)
    {
        Rectangle *newRect = nodeToBeSplitted -> children[i] -> rectangle;
        int rect1_x = (newRect -> x_min + newRect -> x_max) / 2;
        int rect1_y = (newRect -> y_min + newRect -> y_max) / 2;

        if (rect1_x > center_x)
        {
            if (rect1_y > center_y)
            {
                splitX = false;
                corners[2]++;
                indexes[2][i] = i;
            }
            else
            {
                corners[3]++;
                indexes[3][i] = i;
            }
        }
        else
        {
            if (rect1_y > center_y)
            {
                corners[1]++;
                indexes[1][i] = i;
            }
            else
            {
                corners[0]++;
                indexes[0][i] = i;
            }
        }
    }

    // N1 & N2 are the resulting nodes after splitting the nodeToBeSplitted
    Node *N1, *N2, *last_parent;

    // To capture the axis
    float x, y;

    if (splitX)
    {
        if (nodeToBeSplitted -> parent != NULL && nodeToBeSplitted -> parent -> rectangle != NULL)
        {
            x = (nodeToBeSplitted -> rectangle -> x_max + nodeToBeSplitted -> rectangle -> x_min) / 2;
            N1 = createNode(nodeToBeSplitted -> parent, false, nodeToBeSplitted -> rectangle -> x_min, x, nodeToBeSplitted -> rectangle -> y_min, nodeToBeSplitted -> rectangle -> y_max);
            N2 = createNode(nodeToBeSplitted -> parent, false, x, nodeToBeSplitted -> rectangle -> x_max, nodeToBeSplitted -> rectangle -> y_min, nodeToBeSplitted -> rectangle -> y_max);
            last_parent = nodeToBeSplitted -> parent;
        }
        else
        {
            // handle the case where the pointers are null... || root is being split
            x = (nodeToBeSplitted -> rectangle -> x_max + nodeToBeSplitted -> rectangle -> x_min) / 2;
            N1 = createNode(NULL, false, nodeToBeSplitted -> rectangle -> x_min, x, nodeToBeSplitted -> rectangle -> y_min, nodeToBeSplitted -> rectangle -> y_max);
            N2 = createNode(NULL, false, x, nodeToBeSplitted -> rectangle -> x_max, nodeToBeSplitted -> rectangle -> y_min, nodeToBeSplitted -> rectangle -> y_max);
        }
    }

    else
    {
        if (nodeToBeSplitted -> parent != NULL && nodeToBeSplitted -> parent -> rectangle != NULL)
        {
            y = (nodeToBeSplitted -> rectangle -> y_max + nodeToBeSplitted -> rectangle -> y_min) / 2;
            N1 = createNode(nodeToBeSplitted -> parent, false, nodeToBeSplitted -> rectangle -> x_min, nodeToBeSplitted -> rectangle -> x_max, nodeToBeSplitted -> rectangle -> y_min, y);
            N2 = createNode(nodeToBeSplitted -> parent, false, nodeToBeSplitted -> rectangle -> x_min, nodeToBeSplitted -> rectangle -> x_max, y, nodeToBeSplitted -> rectangle -> y_max);
            last_parent = nodeToBeSplitted -> parent;
        }
        else
        {
            // handle the case where the pointers are null...
            y = (nodeToBeSplitted -> rectangle -> y_max + nodeToBeSplitted -> rectangle -> y_min) / 2;
            N1 = createNode(NULL, false, nodeToBeSplitted -> rectangle -> x_min, nodeToBeSplitted -> rectangle -> x_max, nodeToBeSplitted -> rectangle -> y_min, y);
            N2 = createNode(NULL, false, nodeToBeSplitted -> rectangle -> x_min, nodeToBeSplitted -> rectangle -> x_max, y, nodeToBeSplitted -> rectangle -> y_max);
        }
    }

    //Assign children to the new nodes N1 & N2
    if (corners[0] > corners[2])
    {
        for (int i = 0; i < 5; i++)
        {
            if (indexes[0][i] != -1)
            {
                N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[0][i]];
                N1 -> children[(N1 -> num_children)] -> parent = N1;
                N1 -> num_children++;
            }
            if (indexes[2][i] != -1)
            {
                N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[2][i]];
                N2 -> children[(N2 -> num_children)] -> parent = N2;
                N2 -> num_children++;
            }
        }
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            if (indexes[2][i] != -1)
            {
                N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[2][i]];
                N1 -> children[(N1 -> num_children)] -> parent = N1;
                N1 -> num_children++;
            }
            if (indexes[0][i] != -1)
            {
                N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[0][i]];
                N2 -> children[(N2 -> num_children)] -> parent = N2;
                N2 -> num_children++;
            }
        }
    }
    if (corners[1] > corners[3])
    {
        for (int i = 0; i < 5; i++)
        {
            if (indexes[1][i] != -1)
            {
                N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[1][i]];
                N2 -> children[(N2 -> num_children)] -> parent = N2;
                N2 -> num_children++;
            }
            if (indexes[3][i] != -1)
            {
                N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[3][i]];
                N1 -> children[(N1 -> num_children)] -> parent = N1;
                N1 -> num_children++;
            }
        }
    }
    else if (corners[1] < corners[3])
    {
        for (int i = 0; i < 5; i++)
        {
            if (indexes[3][i] != -1)
            {
                N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[3][i]];
                N2 -> children[(N2 -> num_children)] -> parent = N2;
                N2 -> num_children++;
            }
            if (indexes[1][i] != -1)
            {
                N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[1][i]];
                N1 -> children[(N1 -> num_children)] -> parent = N1;
                N1 -> num_children++;
            }
        }
    }

    else
    {
        // Tie Breaker
        // Choose to move objects of C1 OR objects of C3 to N1 according to the least overlap
        float overlap1 = 0, overlap2 = 0;
        for (int i = 0; i < 5; i++)
        {
            if (indexes[1][i] != -1)
            {
                overlap1 += calculateOverlap(N1 -> rectangle, nodeToBeSplitted -> children[indexes[1][i]] -> rectangle);
            }
            if (indexes[3][i] != -1)
            {
                overlap2 += calculateOverlap(N1 -> rectangle, nodeToBeSplitted -> children[indexes[3][i]] -> rectangle);
            }
        }
        if (overlap1 > overlap2)
        {
            for (int i = 0; i < 5; i++)
            {
                if (indexes[1][i] != -1)
                {
                    N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[1][i]];
                    N1 -> children[(N1 -> num_children)] -> parent = N1;
                    N1 -> num_children++;
                }
                if (indexes[3][i] != -1)
                {
                    N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[3][i]];
                    N2 -> children[(N2 -> num_children)] -> parent = N2;
                    N2 -> num_children++;
                }
            }
        }
        else if (overlap1 < overlap2)
        {
            for (int i = 0; i < 5; i++)
            {
                if (indexes[3][i] != -1)
                {
                    N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[3][i]];
                    N1 -> children[(N1 -> num_children)] -> parent = N1;
                    N1 -> num_children++;
                }
                if (indexes[1][i] != -1)
                {
                    N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[1][i]];
                    N2 -> children[(N2 -> num_children)] -> parent = N2;
                    N2 -> num_children++;
                }
            }
        }
        else
        {
            // choose to move objects of C1 OR objects of C3 to N1 according to the least total area covered
            float area1 = 0, area2 = 0;
            for (int i = 0; i < 5; i++)
            {
                if (indexes[1][i] != -1)
                {
                    area1 += calculateArea(nodeToBeSplitted -> children[indexes[1][i]] -> rectangle);
                }
                if (indexes[3][i] != -1)
                {
                    area2 += calculateArea(nodeToBeSplitted -> children[indexes[3][i]] -> rectangle);
                }
            }
            area1 = calculateArea(N1 -> rectangle) - area1;
            area2 = calculateArea(N2 -> rectangle) - area2;
            if (area1 > area2)
            {
                for (int i = 0; i < 5; i++)
                {
                    if (indexes[1][i] != -1)
                    {
                        N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[1][i]];
                        N1 -> children[(N1 -> num_children)] -> parent = N1;
                        N1 -> num_children++;
                    }
                    if (indexes[3][i] != -1)
                    {
                        N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[3][i]];
                        N2 -> children[(N2 -> num_children)] -> parent = N2;
                        N2 -> num_children++;
                    }
                }
            }
            else
            {
                for (int i = 0; i < 5; i++)
                {
                    if (indexes[3][i] != -1)
                    {
                        N1 -> children[(N1 -> num_children)] = nodeToBeSplitted -> children[indexes[3][i]];
                        N1 -> children[(N1 -> num_children)] -> parent = N1;
                        N1 -> num_children++;
                    }
                    if (indexes[1][i] != -1)
                    {
                        N2 -> children[(N2 -> num_children)] = nodeToBeSplitted -> children[indexes[1][i]];
                        N2 -> children[(N2 -> num_children)] -> parent = N2;
                        N2 -> num_children++;
                    }
                }
            }
        }
    }

    // If N1 has lesser children
    if (N1 -> num_children < m)
    {
        int k = N1 -> num_children;
        // So that there's a loop till the minimum criteria is satisfied
        while (k < m)
        {
            if (!splitX)
            {
                // Calculate which child of N1 is closes to the splitting X-axis and transfer that to N2 & increment k
                float minDif = 1e9;
                // To calculate index corresponding to which the child is being transferred from N2 to N1
                int posOfN2Child = -1;
                for (int i = 0; i < N2 -> num_children; i++)
                {
                    float value = (N2 -> children[i] -> rectangle -> y_min + N2 -> children[i] -> rectangle -> y_max) / 2;
                    if (abs(value - y) < minDif)
                    {
                        posOfN2Child = i;
                        minDif = abs(value - y);
                    }
                }

                // at this point, we have the position of the child which has to be shifted.
                N1 -> children[k] = N2 -> children[posOfN2Child];
                N1 -> num_children++;
                int i;
                for (i = posOfN2Child; i < N2 -> num_children - 1; i++)
                {
                    N2 -> children[i] = N2 -> children[i + 1];
                }
                N2 -> num_children--;
                N2 -> children[i] = NULL;
                k++;
            }
            else
            {
                // Calculate which child of N1 is closes to the splitting Y-axis and transfer that to N1 & increment k
                float minDif = 1e9;                
                // To calculate index corresponding to which the child is being transferred from N2 to N1
                int posOfN2Child = -1;
                for (int i = 0; i < N2 -> num_children; i++)
                {
                    float value = (N2 -> children[i] -> rectangle -> x_min + N2 -> children[i] -> rectangle -> x_max) / 2;
                    if (abs(value - x) < minDif)
                    {
                        posOfN2Child = i;
                        minDif = abs(value - x);
                    }
                }
                // at this point, we have the position of the child which has to be shifted.
                N1 -> children[k] = N2 -> children[posOfN2Child];
                N1 -> num_children++;
                int i;
                for (i = posOfN2Child; i < N2 -> num_children - 1; i++)
                {
                    N2 -> children[i] = N2 -> children[i + 1];
                }
                N2 -> num_children--;
                N2 -> children[i] = NULL;
                k++;
            }
        }
    }

    // If N2 has lesser children
    if (N2 -> num_children < m)
    {
        int k = N2 -> num_children;
        // So that there's a loop till the minimum criteria is satisfied
        while (k < m)
        {
            if (!splitX)
            {
                // Calculate which child of N1 is closes to the splitting X-axis and transfer that to N2 & increment k
                float minDif = 1e9;                
                // To calculate index corresponding to which the child is being transferred from N1 to N2
                int posOfN1Child = -1;
                for (int i = 0; i < N1 -> num_children; i++)
                {
                    float value = (N1 -> children[i] -> rectangle -> y_min + N1 -> children[i] -> rectangle -> y_max) / 2;
                    if (abs(value - y) < minDif)
                    {
                        posOfN1Child = i;
                        minDif = abs(value - y);
                    }
                }

                // at this point, we have the position of the child which has to be shifted.
                N2 -> children[k] = N1 -> children[posOfN1Child];
                N2 -> num_children++;
                int i;
                for (i = posOfN1Child; i < N1 -> num_children - 1; i++)
                {
                    N1 -> children[i] = N1 -> children[i + 1];
                }
                N1 -> num_children--;
                N1 -> children[i] = NULL;
                k++;
            }
            else
            {
                // Calculate which child of N1 is closes to the splitting Y-axis and transfer that to N1 & increment k
                float minDif = 1e9;
                // To calculate index corresponding to which the child is being transferred from N1 to N2s
                int posOfN1Child = -1;
                for (int i = 0; i < N1 -> num_children; i++)
                {
                    float value = (N1 -> children[i] -> rectangle -> x_min + N1 -> children[i] -> rectangle -> x_max) / 2;
                    if (abs(value - x) < minDif)
                    {
                        posOfN1Child = i;
                        minDif = abs(value - x);
                    }
                }
                // at this point, we have the position of the child which has to be shifted.
                N2 -> children[k] = N1 -> children[posOfN1Child];
                N2 -> num_children++;
                int i;
                for (i = posOfN1Child; i < N1 -> num_children - 1; i++)
                {
                    N1 -> children[i] = N1 -> children[i + 1];
                }
                N1 -> num_children--;
                N1 -> children[i] = NULL;
                k++;
            }
        }
    }
    
    if (N1 -> parent == NULL && N2 -> parent == NULL)
    {
        // Calculate the new root & Assign N1 & N2 to be its children
        root -> children = (Node **)malloc(sizeof(Node *) * M);
        root -> num_children = 2;
        root -> children[0] = N1;
        root -> children[1] = N2;
        root -> parent = NULL;
        N1 -> parent = root;
        N2 -> parent = root;
        Rectangle *rect1 = N1 -> children[0] -> rectangle;
        Node *temp1 = createNode(NULL, false, rect1 -> x_min, rect1 -> x_max, rect1 -> y_min, rect1 -> y_max);
        Rectangle *rect2 = N2 -> children[0] -> rectangle;
        Node *temp2 = createNode(NULL, false, rect2 -> x_min, rect2 -> x_max, rect2 -> y_min, rect2 -> y_max);
        for (int i = 1; i < N1 -> num_children; i++)
        {
            temp1 -> rectangle = newOverlappingRectangle(temp1 -> rectangle, N1 -> children[i] -> rectangle);
        }
        N1 -> rectangle = temp1 -> rectangle;
        for (int i = 1; i < N2 -> num_children; i++)
        {
            temp2 -> rectangle = newOverlappingRectangle(temp2 -> rectangle, N2 -> children[i] -> rectangle);
        }
        N2 -> rectangle = temp2 -> rectangle;
        root -> rectangle = newOverlappingRectangle(N1 -> rectangle, N2 -> rectangle);
        root -> is_leaf = false;
        return root;
    }

    // If the node being splitted is not the root
    else
    {
        Node *parent = nodeToBeSplitted -> parent;
        int childIndex = -1;
        for (int i = 0; i < parent -> num_children; i++)
        {
            if (parent -> children[i] == nodeToBeSplitted)
            {
                childIndex = i;
                break;
            }
        }
        N1 -> parent = parent;
        N2 -> parent = parent;
        parent -> children[childIndex] = N1;
    }

    adjustTree(root, N1, N2);
    return N2;
}

// To calculate the new rectangle (MBR) covering both the Input Rectangles
Rectangle *newOverlappingRectangle(Rectangle *originalRect, Rectangle *toBeInsertedRect)
{
    Rectangle *rect = (Rectangle *)malloc(sizeof(Rectangle));
    if (originalRect -> x_max < toBeInsertedRect -> x_max)
    {
        rect -> x_max = toBeInsertedRect -> x_max;
    }
    else
    {
        rect -> x_max = originalRect -> x_max;
    }

    if (originalRect -> x_min < toBeInsertedRect -> x_min)
    {
        rect -> x_min = originalRect -> x_min;
    }
    else
    {
        rect -> x_min = toBeInsertedRect -> x_min;
    }

    if (originalRect -> y_max < toBeInsertedRect -> y_max)
    {
        rect -> y_max = toBeInsertedRect -> y_max;
    }
    else
    {
        rect -> y_max = originalRect -> y_max;
    }

    if (originalRect -> y_min < toBeInsertedRect -> y_min)
    {
        rect -> y_min = originalRect -> y_min;
    }
    else
    {
        rect -> y_min = toBeInsertedRect -> y_min;
    }

    return rect;
}

Node *chooseLeaf(Node *N, Rectangle *E)
{
    // CL2 [Leaf check]
    if (N -> is_leaf || N -> children[0] -> is_leaf)
    {
        return N;
    }

    // CL3 [Choose subtree]
    Node *F = N -> children[0];
    float minEnlargement = calculateEnlargement(F -> rectangle, E);

    for (int i = 1; i < N -> num_children; i++)
    {
        float enlargement = calculateEnlargement(N -> children[i] -> rectangle, E);
        if (enlargement < minEnlargement ||
            (enlargement == minEnlargement && calculateArea(N -> children[i] -> rectangle) < calculateArea(F -> rectangle)))
        {
            F = N -> children[i];
            minEnlargement = enlargement;
        }
    }

    // CL4 [Descend until a leaf is reached]
    return chooseLeaf(F, E);
}

// Calculate the enlargement of a rectangle required to engulf other rectangle
double calculateEnlargement(Rectangle *rectangle, Rectangle *other)
{
    double originalArea = calculateArea(rectangle);
    double overlappingArea = calculateOverlap(rectangle, other);
    double enlargementRequired = abs(originalArea - overlappingArea);

    return enlargementRequired;
}

// Calculate the Overlap of two rectangles
double calculateOverlap(Rectangle *originalRect, Rectangle *toBeInsertedRect)
{
    Rectangle *rect = (Rectangle *)malloc(sizeof(Rectangle));
    if (originalRect -> x_max < toBeInsertedRect -> x_max)
    {
        rect -> x_max = toBeInsertedRect -> x_max;
    }
    else
    {
        rect -> x_max = originalRect -> x_max;
    }

    if (originalRect -> x_min < toBeInsertedRect -> x_min)
    {
        rect -> x_min = originalRect -> x_min;
    }
    else
    {
        rect -> x_min = toBeInsertedRect -> x_min;
    }
    if (originalRect -> y_max < toBeInsertedRect -> y_max)
    {
        rect -> y_max = toBeInsertedRect -> y_max;
    }
    else
    {
        rect -> y_max = originalRect -> y_max;
    }

    if (originalRect -> y_min < toBeInsertedRect -> y_min)
    {
        rect -> y_min = originalRect -> y_min;
    }
    else
    {
        rect -> y_min = toBeInsertedRect -> y_min;
    }
    return calculateArea(rect);
}

void printRect(Node *root)
{
    printf("X Min : %f\n", root -> rectangle -> x_min);
    printf("X Max : %f\n", root -> rectangle -> x_max);
    printf("Y Min : %f\n", root -> rectangle -> y_min);
    printf("Y Max : %f\n", root -> rectangle -> y_max);
}

void printData(Node *root)
{
    printf("X  : %f\n", root -> rectangle -> x_min);
    printf("Y  : %f\n", root -> rectangle -> y_min);
}

void printTree(Node *temp)
{
    Node *root = temp;
    if (root -> parent == NULL)
    {
        printf("Root Node with %d children\n", root -> num_children);
        printf("Bounds of the root node : \n");
        printRect(root);
        for (int i = 0; i < root -> num_children; i++)
        {
            printTree(root -> children[i]);
        }
    }
    else if (root -> is_leaf)
    {
        printf("Leaf Node\n");
        printData(root);
    }
    else
    {
        printf("Internal Node with %d children\n", root -> num_children);
        printf("Bounds of this node : \n");
        printRect(root);
        for (int i = 0; i < root -> num_children; i++)
        {
            printTree(root -> children[i]);
        }
    }
}


// To read from the Input File
void readFromFile(char *filename, Node *root)
{
    FILE *f1 = fopen(filename, "r");
    if (f1 == NULL)
    {
        printf("Cannot Open File");
    }
    char line[20];
    char *token;
    int i = 0;
    while (fgets(line, 20, f1))
    {
        token = strtok(line, " ");
        float x = atof(token);
        token = strtok(NULL, "\n");
        float y = atof(token);
        Rectangle *rect = createRectangle(x, x, y, y);
        insert(root, rect);
        i++;
    }
    fclose(f1);
}

bool RectBelongs(Rectangle *nodeRect, Rectangle *searchRect)
{
    if ((nodeRect -> x_min > searchRect -> x_min) &
        (nodeRect -> x_max < searchRect -> x_max) &
        (nodeRect -> y_min > searchRect -> y_min) &
        (nodeRect -> y_max < searchRect -> y_max))
    {
        return true;
    } // checking if the point is in the Search rectangle
    return false;
}

// Function that searches the tree for a given rectangle and stores in the array that is passed to it.
void search_node(Node *root, Rectangle *rectangle, Node **array, int counter)
{
    if (root -> is_leaf)
    {
        bool ans = RectBelongs(root -> rectangle, rectangle);
        if (ans)
        {
            array = (Node **)(realloc(array, sizeof(Node *) * (counter + 1)));
            array[counter++] = root;
        }
    }
    else
    {
        for (int i = 0; i < root -> num_children; i++)
        {
            search_node(root -> children[i], rectangle, array, counter);
        }
    }
}

int main()
{
    char fileName[20] = "data1.txt";

    FILE *f1 = fopen(fileName, "r");

    if (f1 == NULL)
    {
        printf("Cannot Open File");
    }

    char line[20];
    char *token;
    fgets(line, 20, f1);
    token = strtok(line, " ");
    float x = atof(token);
    token = strtok(NULL, "\n");
    float y = atof(token);
    Node *root = createNode(NULL, true, x, x, y, y);
    fclose(f1);

    readFromFile(fileName, root);
    
    printTree(root);
    return 0;
}