#ifndef _FLTK_PREVIEW_H_
#define _FLTK_PREVIEW_H_

#include <map>
#include <string>

#include "core/options.hpp"
#include "preview/preview.hpp"

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Image.H>

class FltkPreview : public Preview
{
public:
	FltkPreview(int x, int y, int w, int h);
	FltkPreview(Options const *options);
	~FltkPreview();
	virtual void SetInfoText(const std::string &text) override;
	// Display the buffer. You get given the fd back in the BufferDoneCallback
	// once its available for re-use.
	virtual void Show(int fd, libcamera::Span<uint8_t> span, StreamInfo const &info) override;
	// Reset the preview window, clearing the current buffers and being ready to
	// show new ones.
	virtual void Reset() override;
	// Check if the window manager has closed the preview.
	virtual bool Quit() override;
	// Return the maximum image size allowed.
	virtual void MaxImageSize(unsigned int &w, unsigned int &h) const override
	{
		w = max_image_width_;
		h = max_image_height_;
	}
	
    virtual void getWindowPos(int &x, int& y) override;
    
    virtual libcamera::PixelFormat getDesiredFormat() override { return libcamera::formats::BGR888; }
    //virtual libcamera::PixelFormat getDesiredFormat() override { return libcamera::formats::RGB888; }
    
    void showMe() { _win->show(); }

private:
	struct Buffer
	{
		Buffer() : fd(-1) {}
		int fd;
		size_t size;
		StreamInfo info;
		//GLuint texture;
	};
	void makeWindow(char const *name);
	void makeBuffer(int fd, size_t size, StreamInfo const &info, Buffer &buffer);
	std::map<int, Buffer> buffers_; // map the DMABUF's fd to the Buffer
	int last_fd_;
	bool first_time_;
    
	// size of preview window
	int x_;
	int y_;
	int width_;
	int height_;
	unsigned int max_image_width_;
	unsigned int max_image_height_;
    
    Fl_Double_Window *_win;
    Fl_Box *_box;
};

#endif // _FLTK_PREVIEW_H_
