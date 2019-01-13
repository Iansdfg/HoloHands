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
   public partial class Form1 : Form
   {
      public Form1()
      {
         InitializeComponent();
      }

      protected override void OnKeyDown(KeyEventArgs e)
      {
         Mat m = new Mat("hands_export.jpg", ImreadModes.Grayscale);

         imgOrig.Image = new Bitmap(m.ToMemoryStream());

         Mat mask = m.Threshold(90, 255, ThresholdTypes.Binary);
         mask = 255 - mask; //invert

         img1.Image = new Bitmap(mask.ToMemoryStream());

         Mat final = new Mat();
         m.CopyTo(final, mask);

         img2.Image = new Bitmap(final.ToMemoryStream());

         base.OnKeyDown(e);
      }
   }
}
