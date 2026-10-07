#import <Cocoa/Cocoa.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

@interface OverlayDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property(nonatomic, strong) NSWindow *window;
@property(nonatomic, strong) NSTextView *notes;
@property(nonatomic, strong) NSButton *topmost;
@property(nonatomic, strong) NSSlider *opacity;
@property(nonatomic, strong) NSTextField *opacityLabel;
@property(nonatomic, strong) NSMenuItem *topmostMenuItem;
- (void)toggleTopmost:(id)sender;
- (void)toggleTopmostFromMenu:(id)sender;
- (void)changeOpacity:(id)sender;
- (void)runSelfTest;
@end

@implementation OverlayDelegate
- (void)installMenu {
    NSMenu *bar = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *applicationItem = [[NSMenuItem alloc] initWithTitle:@"Capture Overlay" action:nil keyEquivalent:@""];
    [bar addItem:applicationItem];
    NSMenu *applicationMenu = [[NSMenu alloc] initWithTitle:@"Capture Overlay"];
    [applicationMenu addItemWithTitle:@"Quit Capture Overlay" action:@selector(terminate:) keyEquivalent:@"q"];
    applicationItem.submenu = applicationMenu;

    NSMenuItem *editItem = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
    [bar addItem:editItem];
    NSMenu *edit = [[NSMenu alloc] initWithTitle:@"Edit"];
    [edit addItemWithTitle:@"Undo" action:@selector(undo:) keyEquivalent:@"z"];
    NSMenuItem *redo = [edit addItemWithTitle:@"Redo" action:@selector(redo:) keyEquivalent:@"z"];
    redo.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagShift;
    [edit addItem:NSMenuItem.separatorItem];
    [edit addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
    [edit addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
    [edit addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
    [edit addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
    editItem.submenu = edit;

    NSMenuItem *windowItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
    [bar addItem:windowItem];
    NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    [windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItemWithTitle:@"Close" action:@selector(performClose:) keyEquivalent:@"w"];
    [windowMenu addItem:NSMenuItem.separatorItem];
    self.topmostMenuItem = [windowMenu addItemWithTitle:@"Always on Top" action:@selector(toggleTopmostFromMenu:) keyEquivalent:@"t"];
    self.topmostMenuItem.target = self;
    self.topmostMenuItem.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagShift;
    NSMenuItem *capture = [windowMenu addItemWithTitle:@"Capture Exclusion (Windows only)" action:nil keyEquivalent:@"e"];
    capture.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagShift;
    capture.enabled = NO;
    windowItem.submenu = windowMenu;
    NSApp.mainMenu = bar;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    [self installMenu];
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 620, 460)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                  NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"Capture Overlay — macOS notes companion";
    self.window.contentMinSize = NSMakeSize(540, 320);
    self.window.delegate = self;
    self.window.releasedWhenClosed = NO;

    self.topmost = [NSButton checkboxWithTitle:@"Always on top (⇧⌘T)" target:self action:@selector(toggleTopmost:)];
    self.topmost.accessibilityIdentifier = @"topmost-toggle";
    NSButton *capture = [NSButton checkboxWithTitle:@"Capture exclusion (Windows only)" target:nil action:nil];
    capture.enabled = NO;
    capture.toolTip = @"SetWindowDisplayAffinity is a Windows API. This Mac companion does not exclude its window from capture.";
    NSStackView *toggles = [NSStackView stackViewWithViews:@[self.topmost, capture]];
    toggles.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    toggles.spacing = 18;
    toggles.alignment = NSLayoutAttributeCenterY;

    self.opacityLabel = [NSTextField labelWithString:@"Opacity: 100%"];
    [self.opacityLabel.widthAnchor constraintEqualToConstant:110].active = YES;
    self.opacity = [NSSlider sliderWithValue:100 minValue:30 maxValue:100 target:self action:@selector(changeOpacity:)];
    self.opacity.continuous = YES;
    self.opacity.accessibilityLabel = @"Window opacity";
    NSStackView *opacityRow = [NSStackView stackViewWithViews:@[self.opacityLabel, self.opacity]];
    opacityRow.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    opacityRow.spacing = 12;

    NSTextField *status = [NSTextField wrappingLabelWithString:
        @"Capture exclusion unavailable on macOS in this demo.\nMicrosoft Teams visibility has NOT been verified."];
    status.textColor = NSColor.systemOrangeColor;
    status.font = [NSFont systemFontOfSize:13 weight:NSFontWeightMedium];

    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 590, 270)];
    scroll.hasVerticalScroller = YES;
    scroll.borderType = NSBezelBorder;
    scroll.autohidesScrollers = YES;
    self.notes = [[NSTextView alloc] initWithFrame:scroll.contentView.bounds];
    self.notes.minSize = NSMakeSize(0, 0);
    self.notes.maxSize = NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX);
    self.notes.verticallyResizable = YES;
    self.notes.horizontallyResizable = NO;
    self.notes.autoresizingMask = NSViewWidthSizable;
    self.notes.textContainer.containerSize = NSMakeSize(scroll.contentSize.width, CGFLOAT_MAX);
    self.notes.textContainer.widthTracksTextView = YES;
    self.notes.richText = NO;
    self.notes.allowsUndo = YES;
    self.notes.font = [NSFont systemFontOfSize:20];
    self.notes.textContainerInset = NSMakeSize(12, 12);
    self.notes.string = @"Type your test notes here…\n\nCAPTURE OVERLAY TEST 12345";
    self.notes.accessibilityLabel = @"Editable notes";
    scroll.documentView = self.notes;

    NSString *platform = [NSString stringWithFormat:@"%@\nNotes are temporary and discarded when this window closes.",
        NSProcessInfo.processInfo.operatingSystemVersionString];
    NSTextField *footer = [NSTextField wrappingLabelWithString:platform];
    footer.font = [NSFont systemFontOfSize:11];
    footer.textColor = NSColor.secondaryLabelColor;

    NSStackView *stack = [NSStackView stackViewWithViews:@[toggles, opacityRow, status, scroll, footer]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack.alignment = NSLayoutAttributeLeading;
    stack.spacing = 12;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    NSView *content = self.window.contentView;
    [content addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:16],
        [stack.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-16],
        [stack.topAnchor constraintEqualToAnchor:content.topAnchor constant:16],
        [stack.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-16],
        [opacityRow.widthAnchor constraintEqualToAnchor:stack.widthAnchor],
        [status.widthAnchor constraintEqualToAnchor:stack.widthAnchor],
        [scroll.widthAnchor constraintEqualToAnchor:stack.widthAnchor],
        [scroll.heightAnchor constraintGreaterThanOrEqualToConstant:80],
        [footer.widthAnchor constraintEqualToAnchor:stack.widthAnchor]
    ]];
    [scroll setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationVertical];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.notes];
    [NSApp activateIgnoringOtherApps:YES];
    if ([NSProcessInfo.processInfo.arguments containsObject:@"--self-test"] ||
        [NSProcessInfo.processInfo.arguments containsObject:@"--self-test-window-actions"]) {
        dispatch_async(dispatch_get_main_queue(), ^{ [self runSelfTest]; });
    }
}

- (void)toggleTopmost:(id)sender {
    (void)sender;
    const BOOL enabled = self.topmost.state == NSControlStateValueOn;
    self.window.level = enabled ? NSFloatingWindowLevel : NSNormalWindowLevel;
    self.topmostMenuItem.state = self.topmost.state;
}

- (void)toggleTopmostFromMenu:(id)sender {
    (void)sender;
    [self.topmost performClick:nil];
}

- (void)changeOpacity:(id)sender {
    (void)sender;
    self.window.alphaValue = self.opacity.doubleValue / 100.0;
    self.opacityLabel.stringValue = [NSString stringWithFormat:@"Opacity: %.0f%%", self.opacity.doubleValue];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    (void)sender;
    return YES;
}

- (void)runSelfTest {
    __block int failures = 0;
    void (^check)(BOOL, const char *) = ^(BOOL passed, const char *name) {
        std::printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
        if (!passed) ++failures;
    };
    NSString *test = @"Native editable notes\nUnicode: café 日本語\nTEST 12345";
    [self.notes selectAll:nil];
    [self.notes insertText:test replacementRange:self.notes.selectedRange];
    check([self.notes.string isEqualToString:test], "multiline Unicode editing");
    [self toggleTopmostFromMenu:nil];
    check(self.window.level == NSFloatingWindowLevel && self.topmostMenuItem.state == NSControlStateValueOn,
        "always-on-top enabled through shared menu/shortcut action");
    [self toggleTopmostFromMenu:nil];
    check(self.window.level == NSNormalWindowLevel, "always-on-top disabled");
    self.opacity.doubleValue = 55;
    [self changeOpacity:nil];
    check(std::abs(self.window.alphaValue - 0.55) < 0.001, "window opacity updated");
    self.opacity.doubleValue = 100;
    [self changeOpacity:nil];
    check(std::abs(self.window.alphaValue - 1.0) < 0.001, "window opacity restored");
    [self.window setContentSize:NSMakeSize(800, 600)];
    [self.window.contentView layoutSubtreeIfNeeded];
    check(self.notes.enclosingScrollView.frame.size.width > 700, "notes resize with window");
    if (![NSProcessInfo.processInfo.arguments containsObject:@"--self-test-window-actions"]) {
        std::printf("Native minimize/restore: manual test or --self-test-window-actions required.\n");
        std::printf("Capture exclusion and Teams visibility: NOT TESTED (Windows required).\n");
        std::fflush(stdout);
        std::exit(failures ? 1 : 0);
    }
    // Observe actual completion rather than assuming a fixed animation duration.
    __block BOOL finished = NO;
    __block id minimizeObserver = nil;
    __block id restoreObserver = nil;
    NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
    restoreObserver = [center addObserverForName:NSWindowDidDeminiaturizeNotification
        object:self.window queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
            (void)note;
            check(!self.window.miniaturized, "restore from minimized state");
            finished = YES;
            [NSNotificationCenter.defaultCenter removeObserver:restoreObserver];
            std::printf("Capture exclusion and Teams visibility: NOT TESTED (Windows required).\n");
            std::fflush(stdout);
            std::exit(failures ? 1 : 0);
        }];
    minimizeObserver = [center addObserverForName:NSWindowDidMiniaturizeNotification
        object:self.window queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
            (void)note;
            check(self.window.miniaturized, "native minimize action");
            [NSNotificationCenter.defaultCenter removeObserver:minimizeObserver];
            [self.window deminiaturize:nil];
        }];
    [self.window miniaturize:nil];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (!finished) {
            check(NO, "minimize/restore completion within 10 seconds");
            std::fflush(stdout);
            std::exit(1);
        }
    });
}
@end

int main() {
    @autoreleasepool {
        NSApplication *application = NSApplication.sharedApplication;
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        OverlayDelegate *delegate = [[OverlayDelegate alloc] init];
        application.delegate = delegate;
        [application run];
    }
    return 0;
}
