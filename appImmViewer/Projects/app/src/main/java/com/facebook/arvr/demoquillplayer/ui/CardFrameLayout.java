/**
 * Copyright 2014 Google
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.facebook.arvr.demoquillplayer.ui;

import android.content.Context;
import android.graphics.Outline;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewOutlineProvider;
import android.widget.FrameLayout;
import com.facebook.arvr.demoquillplayer.R;

@SuppressWarnings("UnusedDeclaration")
public class CardFrameLayout extends FrameLayout {
  public CardFrameLayout(Context context, AttributeSet attrs) {
    this(context, attrs, 0);
  }

  public CardFrameLayout(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    super.onSizeChanged(w, h, oldw, oldh);

    final float radius = getResources().getDimensionPixelSize(R.dimen.card_corner_radius);
    final int vw = w;
    final int vh = h;

    final ViewOutlineProvider vop =
        new ViewOutlineProvider() {
          @Override
          public void getOutline(View view, Outline outline) {
            outline.setRoundRect(0, 0, vw, vh, radius);
          }
        };

    setOutlineProvider(vop);
    setClipToOutline(true);
  }
}
