namespace OpenCVTestArea
{
   partial class Hand_ConvexHull
   {
      /// <summary>
      /// Required designer variable.
      /// </summary>
      private System.ComponentModel.IContainer components = null;

      /// <summary>
      /// Clean up any resources being used.
      /// </summary>
      /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
      protected override void Dispose(bool disposing)
      {
         if (disposing && (components != null))
         {
            components.Dispose();
         }
         base.Dispose(disposing);
      }

      #region Windows Form Designer generated code

      /// <summary>
      /// Required method for Designer support - do not modify
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.img2 = new System.Windows.Forms.PictureBox();
         this.img1 = new System.Windows.Forms.PictureBox();
         this.imgOrig = new System.Windows.Forms.PictureBox();
         ((System.ComponentModel.ISupportInitialize)(this.img2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.img1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.imgOrig)).BeginInit();
         this.SuspendLayout();
         // 
         // img2
         // 
         this.img2.Location = new System.Drawing.Point(809, 380);
         this.img2.Name = "img2";
         this.img2.Size = new System.Drawing.Size(800, 600);
         this.img2.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
         this.img2.TabIndex = 0;
         this.img2.TabStop = false;
         // 
         // img1
         // 
         this.img1.Location = new System.Drawing.Point(-6, 380);
         this.img1.Name = "img1";
         this.img1.Size = new System.Drawing.Size(800, 600);
         this.img1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
         this.img1.TabIndex = 1;
         this.img1.TabStop = false;
         // 
         // imgOrig
         // 
         this.imgOrig.Location = new System.Drawing.Point(-6, 12);
         this.imgOrig.Name = "imgOrig";
         this.imgOrig.Size = new System.Drawing.Size(494, 362);
         this.imgOrig.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
         this.imgOrig.TabIndex = 2;
         this.imgOrig.TabStop = false;
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(1622, 1012);
         this.Controls.Add(this.imgOrig);
         this.Controls.Add(this.img1);
         this.Controls.Add(this.img2);
         this.Name = "Form1";
         this.Text = "Form1";
         ((System.ComponentModel.ISupportInitialize)(this.img2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.img1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.imgOrig)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.PictureBox img2;
      private System.Windows.Forms.PictureBox img1;
      private System.Windows.Forms.PictureBox imgOrig;
   }
}

