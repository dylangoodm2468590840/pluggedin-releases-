#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HardwareMaterials
{
public:
    // Generate high-quality procedural brushed metal & industrial powder-coat texture
    static juce::Image createBrushedMetalTexture(int width = 800, int height = 600)
    {
        juce::File file("c:\\Users\\dylan\\Documents\\First plug in\\Source\\UI\\brushed_gunmetal.jpg");
        if (!file.existsAsFile())
        {
            file = juce::File::getCurrentWorkingDirectory().getChildFile("Source/UI/brushed_gunmetal.jpg");
        }

        if (file.existsAsFile())
        {
            juce::Image loadedImg = juce::ImageFileFormat::loadFrom(file);
            if (loadedImg.isValid())
            {
                if (width > 0 && height > 0 && (loadedImg.getWidth() != width || loadedImg.getHeight() != height))
                {
                    return loadedImg.rescaled(width, height, juce::Graphics::highResamplingQuality);
                }
                return loadedImg;
            }
        }

        // Fallback: procedural grain
        juce::Image img(juce::Image::ARGB, width > 0 ? width : 800, height > 0 ? height : 600, true);
        juce::Graphics g(img);

        // Dark gunmetal base fill
        g.fillAll(juce::Colour(0xff121722));

        auto& rand = juce::Random::getSystemRandom();

        // Horizontal brushed metal striations
        for (int y = 0; y < img.getHeight(); ++y)
        {
            float alpha = 0.02f + rand.nextFloat() * 0.06f;
            juce::Colour lineCol = rand.nextBool() ? juce::Colours::white.withAlpha(alpha) : juce::Colours::black.withAlpha(alpha * 1.2f);
            g.setColour(lineCol);
            g.drawHorizontalLine(y, 0.0f, (float)img.getWidth());
        }

        // Fine powder-coat noise grain
        for (int i = 0; i < (img.getWidth() * img.getHeight()) / 10; ++i)
        {
            int px = rand.nextInt(img.getWidth());
            int py = rand.nextInt(img.getHeight());
            float alpha = rand.nextFloat() * 0.05f;
            img.setPixelAt(px, py, juce::Colours::white.withAlpha(alpha));
        }

        return img;
    }

    // 1. Heavy 3D Gunmetal Metal Chassis with Brushed Metal Texture & Top-Lit Studio Overhead Lighting
    static void drawGunmetalChassis(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 8.0f, const juce::Image& texture = juce::Image())
    {
        // Draw base gunmetal top-lit studio lighting gradient
        juce::ColourGradient chassisGrad(
            juce::Colour(0xff263346), bounds.getX(), bounds.getY(),
            juce::Colour(0xff090c12), bounds.getX(), bounds.getBottom(), false);
        chassisGrad.addColour(0.35, juce::Colour(0xff141b26));
        chassisGrad.addColour(0.75, juce::Colour(0xff0e131d));

        g.setGradientFill(chassisGrad);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Overlay cached brushed metal texture
        if (texture.isValid())
        {
            g.setTiledImageFill(texture, 0, 0, 0.60f);
            g.fillRoundedRectangle(bounds, cornerRadius);
        }

        // Top-Left Beveled Highlight Edge
        g.setColour(juce::Colour(0xff455b7c).withAlpha(0.65f));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.8f);

        // Bottom-Right Deep Shadow Edge
        g.setColour(juce::Colour(0xff030406).withAlpha(0.85f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), cornerRadius, 1.2f);
    }

    // 2. Heavy Left & Right Rack Ears with Mounting Bolts
    static void drawRackEars(juce::Graphics& g, juce::Rectangle<float> bounds, float earWidth = 24.0f)
    {
        // Left Rack Ear
        auto leftEar = juce::Rectangle<float>(bounds.getX(), bounds.getY(), earWidth, bounds.getHeight());
        juce::ColourGradient leftGrad(
            juce::Colour(0xff2d3b50), leftEar.getX(), leftEar.getY(),
            juce::Colour(0xff10151f), leftEar.getRight(), leftEar.getY(), false);
        g.setGradientFill(leftGrad);
        g.fillRect(leftEar);

        g.setColour(juce::Colour(0xff05070a));
        g.drawRect(leftEar, 1.0f);

        // Left Mounting Screws
        drawHexBolt(g, leftEar.getCentreX(), leftEar.getY() + 30.0f, 5.0f);
        drawHexBolt(g, leftEar.getCentreX(), leftEar.getBottom() - 30.0f, 5.0f);

        // Right Rack Ear
        auto rightEar = juce::Rectangle<float>(bounds.getRight() - earWidth, bounds.getY(), earWidth, bounds.getHeight());
        juce::ColourGradient rightGrad(
            juce::Colour(0xff10151f), rightEar.getX(), rightEar.getY(),
            juce::Colour(0xff2d3b50), rightEar.getRight(), rightEar.getY(), false);
        g.setGradientFill(rightGrad);
        g.fillRect(rightEar);

        g.setColour(juce::Colour(0xff05070a));
        g.drawRect(rightEar, 1.0f);

        // Right Mounting Screws
        drawHexBolt(g, rightEar.getCentreX(), rightEar.getY() + 30.0f, 5.0f);
        drawHexBolt(g, rightEar.getCentreX(), rightEar.getBottom() - 30.0f, 5.0f);
    }

    // 3. 3D Chamfered Metal Seam Ribs (Channel Column Separators)
    static void drawChamferedSeam(juce::Graphics& g, float x, float topY, float bottomY)
    {
        // Dark Left Shadow Line
        g.setColour(juce::Colour(0xff040508));
        g.drawLine(x - 1.0f, topY, x - 1.0f, bottomY, 1.2f);

        // Bright Right Highlight Bevel Line
        g.setColour(juce::Colour(0xff4d607a).withAlpha(0.65f));
        g.drawLine(x + 1.0f, topY, x + 1.0f, bottomY, 1.0f);
    }

    // 4. Recessed Inset Metal Panel (Level 2 Depth)
    static void drawRecessedPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 6.0f, const juce::Image& texture = juce::Image())
    {
        // Dark inset background
        g.setColour(juce::Colour(0xff0b0e14));
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Overlay subtle texture
        if (texture.isValid())
        {
            g.setTiledImageFill(texture, 0, 0, 0.35f);
            g.fillRoundedRectangle(bounds, cornerRadius);
        }

        // Inner Top/Left Shadow overlay for depth
        juce::ColourGradient topShadow(
            juce::Colour(0xff000000).withAlpha(0.85f), bounds.getX(), bounds.getY(),
            juce::Colour(0xff000000).withAlpha(0.0f), bounds.getX(), bounds.getY() + 12.0f, false);
        g.setGradientFill(topShadow);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Outer Beveled Frame
        g.setColour(juce::Colour(0xff1d2736).withAlpha(0.65f));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.4f);
    }

    // 5. 3D Metallic Hex Bolt / Rivet
    static void drawHexBolt(juce::Graphics& g, float x, float y, float radius = 4.5f)
    {
        // Shadow underneath bolt
        g.setColour(juce::Colour(0xff030406).withAlpha(0.75f));
        g.fillEllipse(x - radius + 1.0f, y - radius + 1.2f, radius * 2.0f, radius * 2.0f);

        // Metallic head gradient
        juce::ColourGradient boltGrad(
            juce::Colour(0xff3d506d), x - radius, y - radius,
            juce::Colour(0xff101520), x + radius, y + radius, false);
        g.setGradientFill(boltGrad);
        g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colour(0xff526a8f));
        g.drawEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // Center slot line
        g.setColour(juce::Colour(0xff06080c));
        g.drawLine(x - radius * 0.6f, y - radius * 0.2f, x + radius * 0.6f, y + radius * 0.2f, 1.2f);
    }

    // 6. Glass Display Sheen & Bezel
    static void drawGlassDisplay(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 6.0f)
    {
        // Dark CRT glass background
        g.setColour(juce::Colour(0xff06090e));
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Recessed Metal Bezel
        g.setColour(juce::Colour(0xff00f0ff).withAlpha(0.35f));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.5f);
        g.setColour(juce::Colour(0xff030406));
        g.drawRoundedRectangle(bounds.expanded(1.0f), cornerRadius + 1.0f, 1.2f);

        // Glass Reflection Diagonal Sheen
        juce::Path sheen;
        sheen.startNewSubPath(bounds.getX(), bounds.getY());
        sheen.lineTo(bounds.getX() + bounds.getWidth() * 0.45f, bounds.getY());
        sheen.lineTo(bounds.getX(), bounds.getY() + bounds.getHeight() * 0.65f);
        sheen.closeSubPath();

        g.setColour(juce::Colours::white.withAlpha(0.045f));
        g.fillPath(sheen);
    }

    // 7. Vertical Side Rack Grab Handle
    static void drawRackHandle(juce::Graphics& g, float x, float y, float width, float height)
    {
        // Handle shadow
        g.setColour(juce::Colour(0xff030406).withAlpha(0.85f));
        g.fillRoundedRectangle(x + 3.0f, y + 3.0f, width, height, width * 0.5f);

        // Metallic Handle Body
        juce::ColourGradient handleGrad(
            juce::Colour(0xff3f5270), x, y,
            juce::Colour(0xff0e131d), x + width, y, false);
        g.setGradientFill(handleGrad);
        g.fillRoundedRectangle(x, y, width, height, width * 0.5f);

        // Metallic Rim Highlight
        g.setColour(juce::Colour(0xff5975a0));
        g.drawRoundedRectangle(x, y, width, height, width * 0.5f, 1.2f);

        // Inner Grip Recess
        g.setColour(juce::Colour(0xff05070a));
        g.fillRoundedRectangle(x + 3.0f, y + 8.0f, width - 6.0f, height - 16.0f, (width - 6.0f) * 0.5f);

        // Top and Bottom Mount Plates
        drawHexBolt(g, x + width * 0.5f, y - 8.0f);
        drawHexBolt(g, x + width * 0.5f, y + height + 8.0f);
    }
};
