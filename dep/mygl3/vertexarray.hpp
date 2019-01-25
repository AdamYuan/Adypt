//
// Created by adamyuan on 3/10/18.
//

#ifndef MYGL3_VERTEXBUFFER_HPP
#define MYGL3_VERTEXBUFFER_HPP

#include <GL/gl3w.h>
#include <vector>
#include "buffer.hpp"

namespace mygl3
{
	class VertexArray
	{
	private:
		GLuint id_{kInvalidOglId};
		const Buffer *vbo_ptr_{}, *ebo_ptr_{};
		GLsizei attr_length_{0};
		GLsizei get_attr_length() const { return 0; }
		template <class... T> GLsizei get_attr_length(GLuint, GLsizei attr_size, T... args) const { return attr_size + get_attr_length(args...); }

		void set_attr_impl(GLsizei) {}
		template <class... T> void set_attr_impl(GLsizei current, GLuint attr_id, GLsizei attr_size, T... args)
		{
			glEnableVertexArrayAttrib(id_, attr_id);
			glVertexArrayAttribFormat(id_, attr_id, attr_size, GL_FLOAT, GL_FALSE, current * sizeof(GLfloat));
			glVertexArrayAttribBinding(id_, attr_id, 0);
			set_attr_impl(current + attr_size, args...);
		}
	public:
		VertexArray() = default;
		VertexArray(VertexArray &&buffer) noexcept :
				id_{buffer.id_}, attr_length_{buffer.attr_length_}, vbo_ptr_{buffer.vbo_ptr_}, ebo_ptr_{buffer.ebo_ptr_}
				{
					buffer.id_ = kInvalidOglId;
				}
		~VertexArray()
		{
			if(IsValidOglId(id_))
				glDeleteVertexArrays(1, &id_);
		}
		VertexArray (const VertexArray&) = delete;
		VertexArray& operator= (const VertexArray&) = delete;
		/*VertexArray &operator=(VertexArray &&buffer) noexcept
		{
			id_ = buffer.id_;
			attr_length_ = buffer.attr_length_;
			ebo_ptr_ = buffer.ebo_ptr_;
			vbo_ptr_ = buffer.vbo_ptr_;
			buffer.id_ = 0;
			return *this;
		}*/

		template<class... T> void Initialize(T... args)
		{
			glCreateVertexArrays(1, &id_);
			attr_length_ = get_attr_length(args...);
			set_attr_impl(0, args...);
		}

		void BindVertices(const Buffer &vbo)
		{
			glVertexArrayVertexBuffer(id_, 0, vbo.Get(), 0, attr_length_ * sizeof(GLfloat));
			vbo_ptr_ = &vbo;
		}

		void BindIndices(const Buffer &ebo)
		{
			glVertexArrayElementBuffer(id_, ebo.Get());
			ebo_ptr_ = &ebo;
		}

		void DrawArrays(GLenum type) const
		{
			glBindVertexArray(id_);
			glDrawArrays(type, 0, vbo_ptr_->GetByteCount() / sizeof(GLfloat) / attr_length_);
		}

		void DrawElements(GLenum type) const
		{
			glBindVertexArray(id_);
			glDrawElements(type, ebo_ptr_->GetByteCount() / sizeof(GLuint), GL_UNSIGNED_INT, nullptr);
		}
	};
}

#endif
