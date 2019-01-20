using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using OpenCvSharp;

namespace OpenCVTestArea
{
   public partial class Hand_ConvexHull : Form
   {
      public Hand_ConvexHull()
      {
         InitializeComponent();
      }

      protected override void OnKeyDown(KeyEventArgs e)
      {
         Mat original = new Mat("hands_export.jpg", ImreadModes.Grayscale);

         Cv2.ImShow("Original", original);

         Mat mask = original.Threshold(90, 255, ThresholdTypes.Binary);
         mask = 255 - mask; //invert

         Mat hands = new Mat();
         original.CopyTo(hands, mask);

         ////Saving image
         //Image img = new Bitmap(hands.ToMemoryStream());
         //img.Save("hand_masked.jpg");

         Mat canny = hands.Canny(200, 250);

         canny = canny.Blur(new Size(6, 6));

         Point[][] contours;
         HierarchyIndex[] hierarchy;

         Cv2.FindContours(canny, out contours, out hierarchy, RetrievalModes.External, ContourApproximationModes.ApproxNone);

         ////Draw contours
         //for (int i = 0; i < conts.Length; i++)
         //{
         //   Mat contMat = new Mat(original.Size(), original.Type());
         //   Cv2.DrawContours(contMat, conts, i, Scalar.AliceBlue);
         //   Cv2.ImShow("Contour " + i.ToString(), contMat);
         //}



         //Filter contours
         var filteredContours = new List<Point[]>();
         foreach (var contour in contours)
         {
            FilterContour(contour, filteredContours);
         }


         Mat hullMat = new Mat(original.Size(), original.Type());

         //Draw filtered contours.
         for (int i = 0; i < filteredContours.Count; i++)
         {
            Cv2.DrawContours(hullMat, filteredContours, i, Scalar.AliceBlue);
         }


         ////Get convex hulls.
         //var hulls = new List<Mat>();
         //foreach (var cont in contours)
         //{
         //   InputArray a = InputArray.Create(cont);
         //   List<int> hullIndices = new List<int>();

         //   OutputArray.Create()
         //   Cv2.ConvexHull(a, hullIndices.ToArray(), false, false);
         //   hulls.Add(hull);

         //   Vec4i[] defects;

         //   Cv2.ConvexityDefects(cont, hull, defects);
         //}

         ////Draw hulls
         //for (int i = 0; i < hulls.Count; i++)
         //{
         //   Cv2.Polylines(hullMat, hulls, true, Scalar.Aqua);
         //}

         Cv2.ImShow("Hull", hullMat);


         Cv2.ImShow("Defects", hands);


         base.OnKeyDown(e);
      }

      void FilterContour(Point[] newContour, List<Point[]> filteredContours)
      {
         Point2f center;
         float radius;
         Cv2.MinEnclosingCircle(newContour, out center, out radius);

         //Ignore small contours.
         if (radius > 10)
         {
            int overlappingIndex = -1;
            for (int i = 0; i < filteredContours.Count; i++)
            {
               if(BoundingRectangleIntersects(newContour, filteredContours[i]))
               {
                  overlappingIndex = i;
                  break;
               }
            }

            if (overlappingIndex == -1)
            {
               filteredContours.Add(newContour);
            }
            else
            {
               filteredContours[overlappingIndex] = filteredContours[overlappingIndex].Concat(newContour).ToArray();
            }
         }
      }

      bool BoundingCircleIntersects(Point[] a, Point[] b)
      {
         Point2f centerA, centerB;
         float radiusA, radiusB;

         Cv2.MinEnclosingCircle(a, out centerA, out radiusA);
         Cv2.MinEnclosingCircle(b, out centerB, out radiusB);

         return centerA.DistanceTo(centerB) <= radiusA + radiusB;
      }

      bool BoundingRectangleIntersects(Point[] a, Point[] b)
      {
         var rectA = Cv2.BoundingRect(a);
         var rectB = Cv2.BoundingRect(b);

         return rectA.IntersectsWith(rectB);
      }
   }
}
