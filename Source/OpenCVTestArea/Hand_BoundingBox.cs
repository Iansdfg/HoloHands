using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using OpenCvSharp;

namespace OpenCVTestArea
{
   public partial class Hand_BoundingBox : Form
   {
      public Hand_BoundingBox()
      {
         InitializeComponent();
      }

      protected override void OnKeyDown(KeyEventArgs e)
      {
         Mat m = new Mat("hands_export.jpg", ImreadModes.Grayscale);

         imgOrig.Image = new Bitmap(m.ToMemoryStream());

         Mat mask = m.Threshold(90, 255, ThresholdTypes.Binary);
         mask = 255 - mask; //invert

         Mat hands = new Mat();
         m.CopyTo(hands, mask);

         //Saving image
         Image img = new Bitmap(hands.ToMemoryStream());
         img.Save("hand_masked.jpg");

         Mat canny = hands.Canny(200, 250);

         canny = canny.Blur(new OpenCvSharp.Size(3, 3));

         OpenCvSharp.Point[][] conts;
         HierarchyIndex[] hierarchy;

         Cv2.FindContours(canny, out conts, out hierarchy, RetrievalModes.Tree, ContourApproximationModes.ApproxNone);

         //Draw contours
         for (int i = 0; i < conts.Length; i++)
         {
            Cv2.DrawContours(hands, conts, i, Scalar.AliceBlue);
         }

         //Convert to bounding rectangles
         var rects = new List<Rect>();
         foreach (var cont in conts)
         {
            rects.Add(Cv2.BoundingRect(cont));
         }

         //Draw bounding rects
         foreach (var rect in rects)
         {
            hands.Rectangle(rect, Scalar.White, 2);
         }

         img1.Image = new Bitmap(hands.ToMemoryStream());

         var filteredRect = new List<Rect>();
         foreach (var rect in rects)
         {
            //Ignore small rects.
            if (rect.Width > 10 && rect.Height > 10)
            {
               //Combine overlapping rectangles
               var intersectIndex = filteredRect.FindIndex(x => x.IntersectsWith(rect));
               if (intersectIndex == -1)
               {
                  filteredRect.Add(rect);
               }
               else
               {
                  filteredRect[intersectIndex] = rect | filteredRect[intersectIndex];
               }
            }
         }

         //Draw combined rects
         foreach (var rect in filteredRect)
         {
            hands.Rectangle(rect, Scalar.White, 5);
         }

         img2.Image = new Bitmap(hands.ToMemoryStream());

         base.OnKeyDown(e);
      }
   }
}
