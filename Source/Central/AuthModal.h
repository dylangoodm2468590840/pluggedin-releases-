#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

/**
 * @class AuthManager
 * @brief Manages local persistent user profile & session authentication.
 */
class AuthManager
{
public:
    static juce::File getProfileFile()
    {
        juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        juce::File pluggedInDir = appDataDir.getChildFile("PluggedIN");
        if (!pluggedInDir.exists())
            pluggedInDir.createDirectory();

        return pluggedInDir.getChildFile("user_profile.json");
    }

    static bool isLoggedIn()
    {
        juce::File f = getProfileFile();
        if (!f.existsAsFile()) return false;

        auto parsed = juce::JSON::parse(f.loadFileAsString());
        if (auto* obj = parsed.getDynamicObject())
        {
            return obj->getProperty("logged_in").toString() == "true";
        }
        return false;
    }

    static juce::String getCurrentProducerName()
    {
        juce::File f = getProfileFile();
        if (!f.existsAsFile()) return "Producer";

        auto parsed = juce::JSON::parse(f.loadFileAsString());
        if (auto* obj = parsed.getDynamicObject())
        {
            juce::String name = obj->getProperty("producer_name").toString();
            if (name.isNotEmpty()) return name;
        }
        return "Producer";
    }

    static void loginUser(const juce::String& email, const juce::String& name = "")
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("logged_in", "true");
        obj->setProperty("email", email);
        obj->setProperty("producer_name", name.isNotEmpty() ? name : email.upToFirstOccurrenceOf("@", false, false));
        obj->setProperty("tier", "PRO ACCOUNT");
        obj->setProperty("login_time", juce::Time::getCurrentTime().toISO8601(true));

        juce::File f = getProfileFile();
        f.replaceWithText(juce::JSON::toString(juce::var(obj), false));
    }

    static void logoutUser()
    {
        juce::File f = getProfileFile();
        if (f.existsAsFile())
            f.deleteFile();
    }
};

/**
 * @class AuthModalComponent
 * @brief Sleek modal dialog for Sign In & Account Registration.
 */
class AuthModalComponent : public juce::Component
{
public:
    std::function<void()> onAuthSuccess;

    AuthModalComponent()
    {
        // Tabs
        signInTabButton.setButtonText("SIGN IN");
        createAccountTabButton.setButtonText("CREATE ACCOUNT");

        signInTabButton.onClick = [this] { isCreateAccountTab = false; updateTabState(); repaint(); };
        createAccountTabButton.onClick = [this] { isCreateAccountTab = true; updateTabState(); repaint(); };

        addAndMakeVisible(signInTabButton);
        addAndMakeVisible(createAccountTabButton);

        // Inputs
        addAndMakeVisible(nameLabel);
        nameLabel.setText("Producer / Artist Name", juce::dontSendNotification);
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7f8b98));

        addAndMakeVisible(nameInput);
        nameInput.setTextToShowWhenEmpty("e.g. Metro Boomin", juce::Colours::grey);
        nameInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff12151c));
        nameInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        nameInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff232832));

        addAndMakeVisible(emailLabel);
        emailLabel.setText("Email Address", juce::dontSendNotification);
        emailLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7f8b98));

        addAndMakeVisible(emailInput);
        emailInput.setTextToShowWhenEmpty("producer@pluggedin.com", juce::Colours::grey);
        emailInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff12151c));
        emailInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        emailInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff232832));

        addAndMakeVisible(passwordLabel);
        passwordLabel.setText("Password", juce::dontSendNotification);
        passwordLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7f8b98));

        addAndMakeVisible(passwordInput);
        passwordInput.setPasswordCharacter('*');
        passwordInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff12151c));
        passwordInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        passwordInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff232832));

        // Submit Button
        addAndMakeVisible(submitButton);
        submitButton.setButtonText("SIGN IN TO PLUGGEDIN");
        submitButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00f0ff).withAlpha(0.2f));
        submitButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));

        submitButton.onClick = [this]
        {
            juce::String email = emailInput.getText().trim();
            juce::String pass = passwordInput.getText().trim();
            juce::String name = nameInput.getText().trim();

            if (email.isEmpty() || pass.isEmpty())
            {
                statusMessage = "Please enter an email and password.";
                repaint();
                return;
            }

            AuthManager::loginUser(email, name);
            if (onAuthSuccess)
                onAuthSuccess();
        };

        updateTabState();
        setSize(420, 360);
    }

    void updateTabState()
    {
        nameLabel.setVisible(isCreateAccountTab);
        nameInput.setVisible(isCreateAccountTab);

        if (isCreateAccountTab)
        {
            submitButton.setButtonText("CREATE PLUGGEDIN ACCOUNT");
            signInTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff14171d));
            signInTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7f8b98));

            createAccountTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00f0ff).withAlpha(0.25f));
            createAccountTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
        }
        else
        {
            submitButton.setButtonText("SIGN IN TO PLUGGEDIN");
            signInTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00f0ff).withAlpha(0.25f));
            signInTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));

            createAccountTabButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff14171d));
            createAccountTabButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7f8b98));
        }
        resized();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Dark Gunmetal Modal Background
        g.fillAll(juce::Colour(0xff0e1116));

        // Header Title
        g.setColour(juce::Colour(0xff00f0ff));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("⚡ PLUGGEDIN AUDIO ACCOUNT", 20, 16, bounds.getWidth() - 40, 24, juce::Justification::centred);

        // Outline
        g.setColour(juce::Colour(0xff232832));
        g.drawRect(bounds, 1.0f);

        // Status Error / Success message
        if (statusMessage.isNotEmpty())
        {
            g.setColour(juce::Colour(0xffff0055));
            g.setFont(juce::Font(11.0f, juce::Font::plain));
            g.drawText(statusMessage, 20, (int)bounds.getHeight() - 68, bounds.getWidth() - 40, 18, juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        signInTabButton.setBounds(24, 52, (bounds.getWidth() - 56) / 2, 32);
        createAccountTabButton.setBounds(24 + (bounds.getWidth() - 56) / 2 + 8, 52, (bounds.getWidth() - 56) / 2, 32);

        int currentY = 96;
        if (isCreateAccountTab)
        {
            nameLabel.setBounds(24, currentY, bounds.getWidth() - 48, 18);
            nameInput.setBounds(24, currentY + 20, bounds.getWidth() - 48, 30);
            currentY += 56;
        }

        emailLabel.setBounds(24, currentY, bounds.getWidth() - 48, 18);
        emailInput.setBounds(24, currentY + 20, bounds.getWidth() - 48, 30);
        currentY += 56;

        passwordLabel.setBounds(24, currentY, bounds.getWidth() - 48, 18);
        passwordInput.setBounds(24, currentY + 20, bounds.getWidth() - 48, 30);
        currentY += 64;

        submitButton.setBounds(24, currentY, bounds.getWidth() - 48, 38);
    }

private:
    bool isCreateAccountTab { false };
    juce::String statusMessage { "" };

    juce::TextButton signInTabButton;
    juce::TextButton createAccountTabButton;

    juce::Label nameLabel;
    juce::TextEditor nameInput;

    juce::Label emailLabel;
    juce::TextEditor emailInput;

    juce::Label passwordLabel;
    juce::TextEditor passwordInput;

    juce::TextButton submitButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuthModalComponent)
};
