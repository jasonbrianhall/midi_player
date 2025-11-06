/**
 * Zenamp - JavaScript Functionality
 * Handles scroll animations, smooth scrolling, navigation tracking, and accessibility
 */

// ============================================================================
// Scroll Reveal Animation
// ============================================================================

/**
 * Reveal elements with 'reveal' class as they come into view
 * This creates a fade-in and slide-up effect for sections
 */
function reveal() {
    const reveals = document.querySelectorAll('.reveal');
    reveals.forEach(element => {
        const windowHeight = window.innerHeight;
        const elementTop = element.getBoundingClientRect().top;
        const elementVisible = 150; // Trigger when element is 150px from bottom of viewport
        
        if (elementTop < windowHeight - elementVisible) {
            element.classList.add('active');
        }
    });
}

// Listen for scroll events and trigger reveal
window.addEventListener('scroll', reveal);

// Initial call on page load to reveal elements that are already visible
document.addEventListener('DOMContentLoaded', () => {
    reveal();
});

// ============================================================================
// Smooth Scroll for Anchor Links
// ============================================================================

/**
 * Enable smooth scrolling for all anchor links (href="#...")
 * Provides a better user experience when navigating to different sections
 */
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
        const href = this.getAttribute('href');
        
        // Don't prevent default if href is just "#" (empty anchor)
        if (href !== '#') {
            e.preventDefault();
            
            const target = document.querySelector(href);
            if (target) {
                target.scrollIntoView({ behavior: 'smooth' });
            }
        }
    });
});

// ============================================================================
// Active Navigation Link Tracking
// ============================================================================

/**
 * Highlight the current section's navigation link based on scroll position
 * Updates the active nav link color as user scrolls through different sections
 */
window.addEventListener('scroll', () => {
    let current = '';
    const sections = document.querySelectorAll('section');
    
    sections.forEach(section => {
        const sectionTop = section.offsetTop;
        // Account for header height (200px) when determining scroll position
        if (scrollY >= sectionTop - 200) {
            current = section.getAttribute('id');
        }
    });

    // Update nav link colors
    document.querySelectorAll('nav a').forEach(link => {
        link.style.color = '';
        if (link.getAttribute('href') === `#${current}`) {
            link.style.color = '#f39c12'; // Highlight color
        }
    });
});

// ============================================================================
// Page Load Event
// ============================================================================

/**
 * Log successful page load
 * Useful for debugging and analytics
 */
window.addEventListener('load', () => {
    console.log('Zenamp index page loaded successfully! 🎵');
});

// ============================================================================
// Keyboard Navigation & Accessibility
// ============================================================================

/**
 * Handle keyboard events for accessibility features
 * Currently reserved for future modal/fullscreen mode functionality
 */
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        // Future: Handle modal/fullscreen close
        // Example: if (isFullscreenOpen) { closeFullscreen(); }
    }
});

// ============================================================================
// Responsive Navigation Enhancements (Optional)
// ============================================================================

/**
 * Add mobile menu toggle functionality (optional enhancement)
 * Uncomment and implement if you add a hamburger menu in HTML
 */

/*
function setupMobileMenu() {
    const hamburger = document.querySelector('.hamburger');
    const nav = document.querySelector('nav');
    
    if (hamburger) {
        hamburger.addEventListener('click', () => {
            nav.classList.toggle('active');
            hamburger.classList.toggle('active');
        });
        
        // Close menu when a link is clicked
        nav.querySelectorAll('a').forEach(link => {
            link.addEventListener('click', () => {
                nav.classList.remove('active');
                hamburger.classList.remove('active');
            });
        });
    }
}

document.addEventListener('DOMContentLoaded', setupMobileMenu);
*/

// ============================================================================
// Utility: Debounce Function (Optional)
// ============================================================================

/**
 * Debounce utility to optimize scroll event listeners
 * Can be used to improve performance if needed
 */
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

// Example usage: Uncomment to optimize scroll listeners
// const optimizedReveal = debounce(reveal, 100);
// window.addEventListener('scroll', optimizedReveal);

// ============================================================================
// Intersection Observer (Modern Alternative to Scroll Reveal)
// ============================================================================

/**
 * Modern approach using Intersection Observer API
 * More performant than scroll event listeners
 * Uncomment to use instead of scroll-based reveal
 */

/*
const observerOptions = {
    threshold: 0.1,
    rootMargin: '0px 0px -50px 0px'
};

const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
        if (entry.isIntersecting) {
            entry.target.classList.add('active');
            // Optional: Stop observing after element is revealed
            // observer.unobserve(entry.target);
        }
    });
}, observerOptions);

// Observe all elements with 'reveal' class
document.querySelectorAll('.reveal').forEach(element => {
    observer.observe(element);
});
*/
